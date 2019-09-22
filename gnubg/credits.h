/* Do not modify this file!  It is created automatically by credits.sh.
   Modify credits.sh instead. 
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
*/

#include <glib/gi18n.h>

typedef struct _credEntry {
	char* Name;
	char* Type;
} credEntry;

typedef struct _credits {
	const char* Title;
	credEntry *Entry;
} credits;

extern credEntry ceAuthors[];
extern credEntry ceContrib[];
extern credEntry ceTranslations[];
extern credEntry ceSupport[];
extern credEntry ceCredits[];

extern credits creditList[];

extern const char aszAUTHORS[];

extern const char aszCOPYRIGHT[];

