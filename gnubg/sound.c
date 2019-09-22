/*
 * sound.c from GAIM.
 *
 * Copyright (C) 1998-1999, Mark Spencer <markster@marko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * File modified by Joern Thyssen <jthyssen@dk.ibm.com> for use with
 * GNU Backgammon.
 *
 * $Id: sound.c,v 1.99 2015/02/08 13:09:27 plm Exp $
 */

#include "config.h"
#include <string.h>
#if USE_GTK
#include "gtkgame.h"
#else
#include "backgammon.h"
#include <glib.h>
#endif

#include "sound.h"
#include "util.h"

#if defined(WIN32)
/* for PlaySound */
#include "windows.h"
#include <mmsystem.h>

#elif HAVE_APPLE_QUICKTIME
#include <QuickTime/QuickTime.h>
#include <pthread.h>
#include "lib/list.h"

static int fQTInitialised = FALSE;
static int fQTPlaying = FALSE;
listOLD movielist;
static pthread_mutex_t mutexQTAccess;


void *
Thread_PlaySound_QuickTime(void *data)
{
    int done = FALSE;

    fQTPlaying = TRUE;

    do {
        listOLD *pl;

        pthread_mutex_lock(&mutexQTAccess);

        /* give CPU time to QT to process all running movies */
        MoviesTask(NULL, 0);

        /* check if there are any running movie left */
        pl = &movielist;
        done = TRUE;
        do {
            listOLD *next = pl->plNext;
            if (pl->p != NULL) {
                Movie *movie = (Movie *) pl->p;
                if (IsMovieDone(*movie)) {
                    DisposeMovie(*movie);
                    free(movie);
                    ListDelete(pl);
                } else
                    done = FALSE;
            }
            pl = next;
        } while (pl != &movielist);

        pthread_mutex_unlock(&mutexQTAccess);
    } while (!done && fQTPlaying);

    fQTPlaying = FALSE;

    return NULL;
}


void
PlaySound_QuickTime(const char *cSoundFilename)
{
    int err;
    FSSpec fsSoundFile;         /* movie file location descriptor */
    short resRefNum;            /* open movie file reference */

    if (!fQTInitialised) {
        pthread_mutex_init(&mutexQTAccess, NULL);
        ListCreate(&movielist);
        fQTInitialised = TRUE;
    }

    /* QuickTime is NOT reentrant in Mac OS (it is in MS Windows!) */
    pthread_mutex_lock(&mutexQTAccess);

    EnterMovies();              /* can be called multiple times */

    err = NativePathNameToFSSpec(cSoundFilename, &fsSoundFile, 0);
    if (err != 0) {
        outputf(_("PlaySound_QuickTime: error #%d, can't find %s.\n"), err, cSoundFilename);
    } else {
        /* open movie (WAV or whatever) file */
        err = OpenMovieFile(&fsSoundFile, &resRefNum, fsRdPerm);
        if (err != 0) {
            outputf(_("PlaySound_QuickTime: error #%d opening %s.\n"), err, cSoundFilename);
        } else {
            /* create movie from movie file */
            Movie *movie = (Movie *) malloc(sizeof(Movie));
            err = NewMovieFromFile(movie, resRefNum, NULL, NULL, 0, NULL);
            CloseMovieFile(resRefNum);
            if (err != 0) {
                outputf(_("PlaySound_QuickTime: error #%d reading %s.\n"), err, cSoundFilename);
            } else {
                /* reset movie timebase */
                TimeRecord t = { 0 };
                t.base = GetMovieTimeBase(*movie);
                SetMovieTime(*movie, &t);
                /* add movie to list of running movies */
                ListInsert(&movielist, movie);
                /* run movie */
                StartMovie(*movie);
            }
        }
    }

    pthread_mutex_unlock(&mutexQTAccess);

    if (!fQTPlaying) {
        /* launch playing thread if necessary */
        int err;
        pthread_t qtthread;
        fQTPlaying = TRUE;
        err = pthread_create(&qtthread, 0L, Thread_PlaySound_QuickTime, NULL);
        if (err == 0)
            pthread_detach(qtthread);
        else
            fQTPlaying = FALSE;
    }
}
#elif HAVE_APPLE_COREAUDIO
/* Apple CoreAudio Sound support 
 * Written by Michael Petch */

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <ApplicationServices/ApplicationServices.h>

static pthread_mutex_t mutexCAAccess;
static AudioFileID audioFile;
static AUGraph theGraph;
static int fCAInitialised = FALSE;
static Float64 fileDuration = 0.0;

#define CoreAudioChkError(func,context,ret) \
	{ \
		int result; \
		if ((result = func)!=0) \
		{ \
			outputf(_("Apple CoreAudio Error (" context "): %d\n"), result); \
		        pthread_mutex_unlock (&mutexCAAccess); \
			return ret; \
		} \
	}
double CoreAudio_PrepareFileAU(AudioUnit * au, AudioStreamBasicDescription * fileFormat, AudioFileID audioFile);
void CoreAudio_MakeSimpleGraph(AUGraph * theGraph, AudioUnit * fileAU,
                               AudioStreamBasicDescription * fileFormat, AudioFileID audioFile);

void
CoreAudio_ShutDown()
{
    AUGraphStop(theGraph);
    AUGraphUninitialize(theGraph);
    AudioFileClose(audioFile);
    AUGraphClose(theGraph);
}

void
CoreAudio_PlayFile_Thread(void *auGraph)
{
    /* Start playing the sound file, and wait for it to complete */
    AUGraphStart(theGraph);
    usleep((int) (1000.0 * 1000.0 * fileDuration));

    CoreAudio_ShutDown();

    /* Shutdown the audio stream */
    pthread_mutex_unlock(&mutexCAAccess);
}

void
CoreAudio_PlayFile(char *const fileName)
{
    pthread_t CAThread;

    /* first time through initialise the mutex */
    if (!fCAInitialised) {
        pthread_mutex_init(&mutexCAAccess, NULL);
        fCAInitialised = TRUE;
    }

    /*  Apparently CoreAudio is not fully reentrant */
    pthread_mutex_lock(&mutexCAAccess);

    /* Open the sound file */
    CFURLRef outInputFileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                                       (const UInt8 *) fileName, strlen(fileName),
                                                                       false);
    if (AudioFileOpenURL(outInputFileURL, kAudioFileReadPermission, 0, &audioFile)) {
        outputf(_("Apple CoreAudio Error, can't find %s\n"), fileName);
        return;
    }

    /* Get properties of the file */
    AudioStreamBasicDescription fileFormat;
    UInt32 propsize = sizeof(AudioStreamBasicDescription);
    CoreAudioChkError(AudioFileGetProperty(audioFile, kAudioFilePropertyDataFormat,
                                           &propsize, &fileFormat), "AudioFileGetProperty Dataformat",);

    /* Setup sound state */
    AudioUnit fileAU;
    memset(&fileAU, 0, sizeof(AudioUnit));
    memset(&theGraph, 0, sizeof(AUGraph));

    /* Setup a simple output graph and AU */
    CoreAudio_MakeSimpleGraph(&theGraph, &fileAU, &fileFormat, audioFile);

    /* Load the file contents */
    fileDuration = CoreAudio_PrepareFileAU(&fileAU, &fileFormat, audioFile);

    if (pthread_create(&CAThread, 0L, (void *)CoreAudio_PlayFile_Thread, NULL) == 0)
        pthread_detach(CAThread);
    else {
        CoreAudio_ShutDown();
        pthread_mutex_unlock(&mutexCAAccess);
    }
}

double
CoreAudio_PrepareFileAU(AudioUnit * au, AudioStreamBasicDescription * fileFormat, AudioFileID audioFile)
{
    UInt64 nPackets;
    UInt32 propsize = sizeof(nPackets);
    CoreAudioChkError(AudioFileGetProperty(audioFile, kAudioFilePropertyAudioDataPacketCount,
                                           &propsize, &nPackets), "AudioFileGetProperty PacketCount", 0.0);

    /* Get playing time in seconds */
    Float64 fileDuration = (nPackets * fileFormat->mFramesPerPacket) / fileFormat->mSampleRate;

    /* Initialize the region */
    ScheduledAudioFileRegion rgn;
    memset(&rgn, 0, sizeof(rgn));
    rgn.mTimeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    rgn.mTimeStamp.mSampleTime = 0;
    rgn.mCompletionProc = NULL;
    rgn.mCompletionProcUserData = NULL;
    rgn.mAudioFile = audioFile;
    rgn.mLoopCount = 1;
    rgn.mStartFrame = 0;
    rgn.mFramesToPlay = (UInt32) (nPackets * fileFormat->mFramesPerPacket);

    CoreAudioChkError(AudioUnitSetProperty(*au, kAudioUnitProperty_ScheduledFileRegion,
                                           kAudioUnitScope_Global, 0, &rgn, sizeof(rgn)),
                      "kAudioUnitProperty_ScheduledFileRegion", 0.0);

    UInt32 defaultVal = 0;
    CoreAudioChkError(AudioUnitSetProperty(*au, kAudioUnitProperty_ScheduledFilePrime,
                                           kAudioUnitScope_Global, 0, &defaultVal, sizeof(defaultVal)),
                      "kAudioUnitProperty_ScheduledFilePrime", 0.0);


    /* Inform AU to start playing at next cycle */
    AudioTimeStamp startTime;
    memset(&startTime, 0, sizeof(startTime));
    startTime.mFlags = kAudioTimeStampSampleTimeValid;
    startTime.mSampleTime = -1;
    CoreAudioChkError(AudioUnitSetProperty(*au, kAudioUnitProperty_ScheduleStartTimeStamp,
                                           kAudioUnitScope_Global, 0, &startTime, sizeof(startTime)),
                      "AudioUnitSetproperty StartTime", 0.0);

    return fileDuration;
}

void
CoreAudio_MakeSimpleGraph(AUGraph * theGraph, AudioUnit * fileAU, AudioStreamBasicDescription * fileFormat,
                          AudioFileID audioFile)
{
    CoreAudioChkError(NewAUGraph(theGraph), "NewAUGraph",);

    AudioComponentDescription cd;
    memset(&cd, 0, sizeof(cd));

    /* Initialize and add Output Node */
    cd.componentType = kAudioUnitType_Output;
    cd.componentSubType = kAudioUnitSubType_DefaultOutput;
    cd.componentManufacturer = kAudioUnitManufacturer_Apple;

    AUNode outputNode;
    CoreAudioChkError(AUGraphAddNode(*theGraph, &cd, &outputNode), "AUGraphAddNode Output",);

    /* Initialize and add the AU node */
    AUNode fileNode;
    cd.componentType = kAudioUnitType_Generator;
    cd.componentSubType = kAudioUnitSubType_AudioFilePlayer;

    CoreAudioChkError(AUGraphAddNode(*theGraph, &cd, &fileNode), "AUGraphAddNode AU",);

    /* Make connections */
    CoreAudioChkError(AUGraphOpen(*theGraph), "AUGraphOpen",);

    /* Set Schedule properties and initialize the graph with the file */
    AudioUnit anAU;
    memset(&anAU, 0, sizeof(anAU));
    CoreAudioChkError(AUGraphNodeInfo(*theGraph, fileNode, NULL, &anAU), "AUGraphNodeInfo",);

    *fileAU = anAU;

    CoreAudioChkError(AudioUnitSetProperty(*fileAU, kAudioUnitProperty_ScheduledFileIDs,
                                           kAudioUnitScope_Global, 0, &audioFile, sizeof(audioFile)),
                      "SetScheduleFile",);
    CoreAudioChkError(AUGraphConnectNodeInput(*theGraph, fileNode, 0, outputNode, 0), "AUGraphConnectNodeInput",);
    CoreAudioChkError(AUGraphInitialize(*theGraph), "AUGraphInitialize",);
}

#elif HAVE_CANBERRA
#include <canberra.h>
#include <canberra-gtk.h>
#endif

const char *sound_description[NUM_SOUNDS] = {
    N_("Starting GNU Backgammon"),
    N_("Exiting GNU Backgammon"),
    N_("Agree"),
    N_("Doubling"),
    N_("Drop"),
    N_("Chequer movement"),
    N_("Move"),
    N_("Redouble"),
    N_("Resign"),
    N_("Roll"),
    N_("Take"),
    N_("Human fans"),
    N_("Human wins game"),
    N_("Human wins match"),
    N_("Bot fans"),
    N_("Bot wins game"),
    N_("Bot wins match"),
    N_("Analysis is finished")
};

const char *sound_command[NUM_SOUNDS] = {
    "start",
    "exit",
    "agree",
    "double",
    "drop",
    "chequer",
    "move",
    "redouble",
    "resign",
    "roll",
    "take",
    "humanfans",
    "humanwinsgame",
    "humanwinsmatch",
    "botfans",
    "botwinsgame",
    "botwinsmatch",
    "analysisfinished"
};

int fSound = TRUE;
int fQuiet = FALSE;
static char *sound_cmd = NULL;

void
playSoundFile(char *file, gboolean UNUSED(sync))
{
    GError *error = NULL;

    if (!g_file_test(file, G_FILE_TEST_EXISTS)) {
        outputf(_("The sound file (%s) doesn't exist.\n"), file);
        return;
    }

    if (sound_cmd && *sound_cmd) {
        char *commandString;

        commandString = g_strdup_printf("%s %s", sound_cmd, file);
        if (!g_spawn_command_line_async(commandString, &error)) {
            outputf(_("sound command (%s) could not be launched: %s\n"), commandString, error->message);
            g_error_free(error);
        }
        return;
    }
#if defined(WIN32)
    SetLastError(0);
    while (!PlaySound(file, NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP | SND_NODEFAULT)) {
        static int soundDeviceAttached = -1;
        if (soundDeviceAttached == -1) {        /* Check for sound card */
            soundDeviceAttached = (int) waveOutGetNumDevs();
        }
        if (!soundDeviceAttached) {     /* No sound card found - disable sound */
            g_print(_("No soundcard found - sounds disabled\n"));
            fSound = FALSE;
            return;
        }
        /* Check for errors */
        if (GetLastError()) {
            PrintSystemError(_("Playing sound"));
            SetLastError(0);
            return;
        }
        Sleep(1);               /* Wait (1ms) for current sound to finish */
    }
#elif HAVE_APPLE_QUICKTIME
    PlaySound_QuickTime(file);
#elif HAVE_APPLE_COREAUDIO
    CoreAudio_PlayFile(file);
#elif HAVE_CANBERRA
    {
        static ca_context *canberracontext = NULL;
        if (!canberracontext) {
#if USE_GTK
            if (fX)
                canberracontext = ca_gtk_context_get();
            else
#endif
                ca_context_create(&canberracontext);
            ca_context_change_props(canberracontext, CA_PROP_CANBERRA_ENABLE, "1", NULL);
        }
        ca_context_play(canberracontext, 0, CA_PROP_MEDIA_FILENAME, file, NULL);
    }
#endif
}

extern void
playSound(const gnubgsound gs)
{
    char *sound;

    if (!fSound || fQuiet)
        /* no sounds for this user */
        return;

    sound = GetSoundFile(gs);
    if (!*sound) {
        g_free(sound);
        return;
    }
#if USE_GTK
    if (!fX || gs == SOUND_EXIT)
        playSoundFile(sound, TRUE);
    else
        playSoundFile(sound, FALSE);
#else
    playSoundFile(sound, TRUE);
#endif

    g_free(sound);
}


extern void
SoundWait(void)
{
    if (!fSound)
        return;
#ifdef WIN32
    /* Wait 1/10 of a second to make sure sound has started */
    Sleep(100);
    while (!PlaySound(NULL, NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP | SND_NODEFAULT))
        Sleep(1);               /* Wait (1ms) for previous sound to finish */
    return;
#endif
}

static char *sound_file[NUM_SOUNDS] = { 0 };

extern char *
GetDefaultSoundFile(gnubgsound sound)
{
    static char aszDefaultSound[NUM_SOUNDS][80] = {
        /* start and exit */
        "fanfare.wav",
        "haere-ra.wav",
        /* commands */
        "drop.wav",
        "double.wav",
        "drop.wav",
        "chequer.wav",
        "move.wav",
        "double.wav",
        "resign.wav",
        "roll.wav",
        "take.wav",
        /* events */
        "dance.wav",
        "gameover.wav",
        "matchover.wav",
        "dance.wav",
        "gameover.wav",
        "matchover.wav",
        "fanfare.wav"
    };

    return BuildFilename2("sounds", aszDefaultSound[sound]);
}

extern char *
GetSoundFile(gnubgsound sound)
{
    if (!sound_file[sound])
        return GetDefaultSoundFile(sound);

    if (!(*sound_file[sound]))
        return g_strdup("");

    if (g_file_test(sound_file[sound], G_FILE_TEST_EXISTS))
        return g_strdup(sound_file[sound]);

    if (g_path_is_absolute(sound_file[sound]))
        return GetDefaultSoundFile(sound);

    return BuildFilename(sound_file[sound]);
}

extern void
SetSoundFile(const gnubgsound sound, const char *file)
{
    char *old_file = GetSoundFile(sound);
    const char *new_file = file ? file : "";
    if (!strcmp(new_file, old_file)) {
        g_free(old_file);
        return;                 /* No change */
    }
    g_free(old_file);

    if (!*new_file) {
        outputf(_("No sound played for: %s\n"), gettext(sound_description[sound]));
    } else {
        outputf(_("Sound for: %s: %s\n"), gettext(sound_description[sound]), new_file);
    }
    g_free(sound_file[sound]);
    sound_file[sound] = g_strdup(new_file);
}

extern const char *
sound_get_command(void)
{
    return (sound_cmd ? sound_cmd : "");
}

extern char *
sound_set_command(const char *sz)
{
    g_free(sound_cmd);
    sound_cmd = g_strdup(sz ? sz : "");
    return sound_cmd;
}

extern void
SetExitSoundOff(void)
{
    sound_file[SOUND_EXIT] = g_strdup("");
}
