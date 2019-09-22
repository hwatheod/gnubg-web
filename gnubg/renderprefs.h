/*
 * renderprefs.h
 *
 * by Gary Wong <gtw@gnu.org>, 2000, 2003
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
 * $Id: renderprefs.h,v 1.11 2013/06/16 02:16:20 mdpetch Exp $
 */

#ifndef RENDERPREFS_H
#define RENDERPREFS_H

#ifndef RENDER_H
#include "render.h"
#endif

extern const char *aszWoodName[];
extern renderdata *GetMainAppearance(void);
extern void CopyAppearance(renderdata * prd);

extern void RenderPreferencesParam(renderdata * prd, const char *szParam, char *szValue);
extern void SaveRenderingSettings(FILE * pf);

#if USE_BOARD3D
char *WriteMaterial(Material * pMat);
char *WriteMaterialDice(renderdata * prd, int num);
#endif

#endif
