/*
 * file.h
 *
 * by Christian Anthon 2007
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
 * $Id: file.h,v 1.10 2013/11/13 23:39:51 plm Exp $
 */

#ifndef FILE_H
#define FILE_H

typedef enum {
    EXPORT_SGF,
    EXPORT_HTML,
    EXPORT_GAM,
    EXPORT_MAT,
    EXPORT_POS,
    EXPORT_LATEX,
    EXPORT_PDF,
    EXPORT_TEXT,
    EXPORT_PNG,
    EXPORT_PS,
    EXPORT_SNOWIETXT,
    EXPORT_SVG,
    N_EXPORT_TYPES
} ExportType;

typedef enum {
    IMPORT_SGF,
    IMPORT_SGG,
    IMPORT_MAT,
    IMPORT_OLDMOVES,
    IMPORT_POS,
    IMPORT_SNOWIETXT,
    IMPORT_TMG,
    IMPORT_EMPIRE,
    IMPORT_PARTY,
    IMPORT_BGROOM,
    N_IMPORT_TYPES
} ImportType;

typedef struct _ExportFormat ExportFormat;
struct _ExportFormat {
    ExportType type;
    const char *extension;
    const char *description;
    const char *clname;
    int exports[3];
};

typedef struct _ImportFormat ImportFormat;
struct _ImportFormat {
    ImportType type;
    const char *extension;
    const char *description;
    const char *clname;
};

extern ExportFormat export_format[];
extern ImportFormat import_format[];

typedef struct _FilePreviewData {
    ImportType type;
} FilePreviewData;

extern char *GetFilename(int CheckForCurrent, ExportType type);
extern FilePreviewData *ReadFilePreview(const char *filename);

#endif
