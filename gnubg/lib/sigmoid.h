/*
 * sigmoid.h
 *
 * by Gary Wong <gtw@gnu.org>, 1998, 1999, 2000, 2001, 2002.
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
 * $Id: sigmoid.h,v 1.6 2014/09/21 20:33:48 plm Exp $
 */

#ifndef SIGMOID_H
#define SIGMOID_H
/* e[k] = exp(k/10) / 10 */
static float e[101] = {
    0.10000000000000001f,
    0.11051709180756478f,
    0.12214027581601698f,
    0.13498588075760032f,
    0.14918246976412702f,
    0.16487212707001281f,
    0.18221188003905089f,
    0.20137527074704767f,
    0.22255409284924679f,
    0.245960311115695f,
    0.27182818284590454f,
    0.30041660239464335f,
    0.33201169227365473f,
    0.36692966676192446f,
    0.40551999668446748f,
    0.44816890703380646f,
    0.49530324243951152f,
    0.54739473917271997f,
    0.60496474644129461f,
    0.66858944422792688f,
    0.73890560989306509f,
    0.81661699125676512f,
    0.90250134994341225f,
    0.99741824548147184f,
    1.1023176380641602f,
    1.2182493960703473f,
    1.3463738035001691f,
    1.4879731724872838f,
    1.6444646771097049f,
    1.817414536944306f,
    2.0085536923187668f,
    2.2197951281441637f,
    2.4532530197109352f,
    2.7112638920657881f,
    2.9964100047397011f,
    3.3115451958692312f,
    3.6598234443677988f,
    4.0447304360067395f,
    4.4701184493300818f,
    4.9402449105530168f,
    5.4598150033144233f,
    6.034028759736195f,
    6.6686331040925158f,
    7.3699793699595784f,
    8.1450868664968148f,
    9.0017131300521811f,
    9.9484315641933776f,
    10.994717245212353f,
    12.151041751873485f,
    13.428977968493552f,
    14.841315910257659f,
    16.402190729990171f,
    18.127224187515122f,
    20.033680997479166f,
    22.140641620418716f,
    24.469193226422039f,
    27.042640742615255f,
    29.886740096706028f,
    33.029955990964865f,
    36.503746786532886f,
    40.34287934927351f,
    44.585777008251675f,
    49.274904109325632f,
    54.457191012592901f,
    60.184503787208222f,
    66.514163304436181f,
    73.509518924197266f,
    81.24058251675433f,
    89.784729165041753f,
    99.227471560502622f,
    109.66331584284585f,
    121.19670744925763f,
    133.9430764394418f,
    148.02999275845451f,
    163.59844299959269f,
    180.80424144560632f,
    199.81958951041173f,
    220.83479918872089f,
    244.06019776244983f,
    269.72823282685101f,
    298.09579870417281f,
    329.44680752838406f,
    364.09503073323521f,
    402.38723938223131f,
    444.7066747699858f,
    491.47688402991344f,
    543.16595913629783f,
    600.29122172610175f,
    663.42440062778894f,
    733.19735391559948f,
    810.3083927575384f,
    895.52927034825075f,
    989.71290587439091f,
    1093.8019208165192f,
    1208.8380730216988f,
    1335.9726829661872f,
    1476.4781565577266f,
    1631.7607198015421f,
    1803.3744927828525f,
    1993.0370438230298f,
    1993.0370438230298f         /* one extra :-) */
};

/* Calculate an approximation to the sigmoid function 1 / ( 1 + e^x ).
 * This is executed very frequently during neural net evaluation, so
 * careful optimisation here pays off.
 * 
 * Statistics on sigmoid(x) calls:
 * * >99% of the time, x is positive.
 * *  82% of the time, 3 < abs(x) < 8.
 * 
 * The Intel x87's `f2xm1' instruction makes calculating accurate
 * exponentials comparatively fast, but still about 30% slower than
 * the lookup table used here. */

static inline float
sigmoid(float const xin)
{
    if (xin >= 0.0f) { 
        /* xin is almost always positive; we place this branch of the `if'
         * first, in the hope that the compiler/processor will predict the
         * conditional branch will not be taken. */
        if (xin < 10.0f) {
            /* again, predict the branch not to be taken */
            const float x1 = 10.0f * xin;
            const int i = (int) x1;

            return 1 / (1 + e[i] * ((10 - i) + x1));
        } else
            return 1.0f / 19931.370438230298f;
    } else {
        if (xin > -10.0f) {
            const float x1 = -10.0f * xin;
            const int i = (int) x1;

            return 1 - 1 / (1 + e[i] * ((10 - i) + x1));
        } else
            return 19930.370438230298f / 19931.370438230298f;
    }
}


#endif
