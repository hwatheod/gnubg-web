/*
 * format.h
 *
 * by Joern Thyssen <jth@gnubg.org>, 2003
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
 * $Id: format.h,v 1.13 2013/06/16 02:16:12 mdpetch Exp $
 */

#ifndef FORMAT_H
#define FORMAT_H

extern int fOutputMWC, fOutputWinPC, fOutputMatchPC;
extern unsigned int fOutputDigits;
extern float rErrorRateFactor;

/* misc. output routines used by text and HTML export */

extern char *OutputPercents(const float ar[], const int f);

extern char *OutputPercent(const float r);

extern char *OutputMWC(const float r, const cubeinfo * pci, const int f);

extern char *OutputEquity(const float r, const cubeinfo * pci, const int f);

extern char *OutputRolloutContext(const char *szIndent, const rolloutcontext * prc);

extern char *OutputEvalContext(const evalcontext * pec, const int fChequer);

extern char *OutputEquityDiff(const float r1, const float r2, const cubeinfo * pci);

extern char *OutputEquityScale(const float r, const cubeinfo * pci, const cubeinfo * pciBase, const int f);

extern char *OutputRolloutResult(const char *szIndent,
                                 char asz[][1024],
                                 float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                                 float aarStdDev[][NUM_ROLLOUT_OUTPUTS],
                                 const cubeinfo aci[], const int alt, const int cci, const int fCubeful);

extern char *OutputCubeAnalysisFull(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                                    float aarStdDev[2][NUM_ROLLOUT_OUTPUTS],
                                    const evalsetup * pes, const cubeinfo * pci,
                                    int fDouble, int fTake, skilltype stDouble, skilltype stTake);

extern char *OutputCubeAnalysis(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                                float aarStdDev[2][NUM_ROLLOUT_OUTPUTS], const evalsetup * pes, const cubeinfo * pci);

extern char *OutputMoneyEquity(const float ar[], const int f);

extern char *FormatCubePosition(char *sz, cubeinfo * pci);
extern void
 FormatCubePositions(const cubeinfo * pci, char asz[2][40]);

#endif                          /* FORMAT_H */
