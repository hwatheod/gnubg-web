/*
 * makebearoff.c
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1997-1999.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
 * published by the Free Software Foundation.
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
 * $Id: bearoffdump.c,v 1.22 2015/02/08 15:09:09 plm Exp $
 */

#include "config.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "positionid.h"
#include "bearoff.h"
#include "multithread.h"
#include "backgammon.h"

extern void
MT_CloseThreads(void)
{
    return;
}

extern int
main(int argc, char **argv)
{

    char *filename, *szPosID = NULL;
    unsigned int id = 0;
    bearoffcontext *pbc;
    char sz[4096];
    TanBoard anBoard;

    GOptionEntry ao[] = {
        {"index", 'n', 0, G_OPTION_ARG_INT, &id,
         "index", NULL},
        {"posid", 'p', 0, G_OPTION_ARG_STRING, &szPosID,
         "Position ID", NULL},
        {NULL, 0, 0, 0, NULL, NULL, NULL}
    };
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new("file");
    g_option_context_add_main_entries(context, ao, PACKAGE);
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);
    if (error) {
        g_printerr("%s\n", error->message);
        exit(EXIT_FAILURE);
    }

    if ((szPosID && id) || (!szPosID && !id)) {
        g_printerr("Either Position ID or index is required. Not Both.\n" "For more help try `bearoffdump --help'\n");
        exit(EXIT_FAILURE);
    }

    if (argc != 2) {
        g_printerr("A bearoff database file should be given as an argument\n"
                   "For more help try `bearoffdump --help'\n");
        exit(EXIT_FAILURE);
    }
    filename = argv[1];

    printf("Bearoff database: %s\n", filename);
    if (!id) {
        printf("Position ID     : %s\n", szPosID);
    } else {
        printf("Position number : %u\n", id);
    }

    /* This is needed since we call ReadBearoffFile() from bearoff.c */
    MT_InitThreads();

    if (!(pbc = BearoffInit(filename, BO_NONE, NULL))) {
        printf("Failed to initialise bearoff database %s\n", filename);
        exit(-1);
    }

    /* information about bearoff database */

    printf("\n" "Information about database:\n\n");

    *sz = 0;
    BearoffStatus(pbc, sz);
    puts(sz);

    /* set up board */

    memset(anBoard, 0, sizeof anBoard);

    if (!id) {
        printf("\n" "Dump of position ID: %s\n\n", szPosID);

        PositionFromID(anBoard, szPosID);
    } else {
        unsigned int n, nUs, nThem;

        printf("\n" "Dump of position#: %u\n\n", id);

        n = Combination(pbc->nPoints + pbc->nChequers, pbc->nPoints);
        nUs = id / n;
        nThem = id % n;
        PositionFromBearoff(anBoard[0], nThem, pbc->nPoints, pbc->nChequers);
        PositionFromBearoff(anBoard[1], nUs, pbc->nPoints, pbc->nChequers);
    }

    /* dump req. position */

    *sz = 0;
    BearoffDump(pbc, (ConstTanBoard) anBoard, sz);
    puts(sz);

    BearoffClose(pbc);

    return 0;

}
