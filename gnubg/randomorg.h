/*
 * randomorg.h
 *
 * by Michael Petch <mpetch@capp-sysware.com>, 2015.
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
 * $Id: randomorg.h,v 1.2 2015/01/19 17:22:59 mdpetch Exp $
 */

#ifndef RANDOMORG_H
#define RANDOMORG_H

#include <stdlib.h>

/*#define RANDOMORG_DEBUG 1 */

#define SUPPORT_EMAIL "support@gnubg.org"
#define STRINGIZEAUX(num) #num
#define STRINGIZE(num) STRINGIZEAUX(num)

#define RANDOMORGSITE "www.random.org"
#define BUFLENGTH 500
#define BUFLENGTH_STR STRINGIZE(BUFLENGTH)

#define RANDOMORG_URL "https://" RANDOMORGSITE "/integers/?num=" \
	BUFLENGTH_STR "&min=0&max=5&col=1&base=10&format=plain&rnd=new"
#define RANDOMORG_USERAGENT "GNUBG/" VERSION " (email: " SUPPORT_EMAIL ")"
#define RANDOMORG_CERTPATH ".\\etc\\ssl\\"
#define RANDOMORG_CABUNDLE "ca-bundle.crt"

typedef struct _RandomData {
    size_t nNumRolls;
    int nCurrent;
    unsigned int anBuf[BUFLENGTH];
} RandomData;

extern unsigned int getDiceRandomDotOrg(void);

#endif
