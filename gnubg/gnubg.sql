--
-- gnubg.sql
--
-- by Joern Thyssen <jth@gnubg.org>, 2004.
--
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
--
-- $Id: gnubg.sql,v 1.14 2013/08/21 03:45:22 mdpetch Exp $
--

-- Table: control
-- Holds the most current value of xxx_id for every table.

CREATE TABLE control (
    tablename     CHAR(80) NOT NULL
   ,next_id       INTEGER NOT NULL
   ,PRIMARY KEY (tablename)
);

CREATE UNIQUE INDEX icontrol ON control (
    tablename
);

-- Table: player
-- a player

CREATE TABLE player (
   player_id INTEGER NOT NULL
   -- Name of player
   ,name          CHAR(80) NOT NULL
   -- Misc notes about this player
   ,notes         VARCHAR(1000) NOT NULL
   ,PRIMARY KEY (player_id)
);

CREATE UNIQUE INDEX iplayer ON player (
    player_id
);

-- Table: session

CREATE TABLE session (
    session_id        INTEGER NOT NULL
   ,checksum        CHAR(33) NOT NULL
   -- Player 0
   ,player_id0      INTEGER NOT NULL 
   -- Player 1
   ,player_id1      INTEGER NOT NULL 
   -- The result of the match/session:
   -- - the total number of points won or lost
   -- - +1/0/-1 for player 0 won the match, match not complete, and
   --   player 1 won match, respectively
   ,result          INTEGER NOT NULL
   -- Length of match
   -- 0=session, >0=match
   ,length          INTEGER NOT NULL
   -- Timestamp for insert into database
   ,added           TIMESTAMP NOT NULL
   -- Match info
   ,rating0         CHAR(80) NOT NULL
   ,rating1         CHAR(80) NOT NULL
   ,event           CHAR(80) NOT NULL
   ,round           CHAR(80) NOT NULL
   ,place           CHAR(80) NOT NULL
   ,annotator       CHAR(80) NOT NULL
   ,comment         CHAR(80) NOT NULL
   ,date            DATE 
   ,PRIMARY KEY (session_id)
   ,FOREIGN KEY (player_id0) REFERENCES player (player_id)
      ON DELETE RESTRICT
   ,FOREIGN KEY (player_id1) REFERENCES player (player_id)
      ON DELETE RESTRICT
);

CREATE UNIQUE INDEX isession ON session (
    session_id
);

-- Table: statistics
-- Used from session and game tables to store match and game statistics,
-- respectively.

CREATE TABLE matchstat (
    matchstat_id        INTEGER NOT NULL
   -- session identification
   ,session_id                          INTEGER NOT NULL
   -- player identification
   ,player_id                         INTEGER NOT NULL
   -- chequerplay statistics
   ,total_moves                       INTEGER NOT NULL 
   ,unforced_moves                    INTEGER NOT NULL
   ,unmarked_moves                    INTEGER NOT NULL
   ,good_moves                        INTEGER NOT NULL
   ,doubtful_moves                    INTEGER NOT NULL
   ,bad_moves                         INTEGER NOT NULL
   ,very_bad_moves                    INTEGER NOT NULL
   ,chequer_error_total_normalised    FLOAT   NOT NULL
   ,chequer_error_total               FLOAT   NOT NULL
   ,chequer_error_per_move_normalised FLOAT   NOT NULL
   ,chequer_error_per_move            FLOAT   NOT NULL
   ,chequer_rating                    INTEGER NOT NULL
   -- luck statistics
   ,very_lucky_rolls                  INTEGER NOT NULL
   ,lucky_rolls                       INTEGER NOT NULL
   ,unmarked_rolls                    INTEGER NOT NULL
   ,unlucky_rolls                     INTEGER NOT NULL
   ,very_unlucky_rolls                INTEGER NOT NULL
   ,luck_total_normalised             FLOAT   NOT NULL
   ,luck_total                        FLOAT   NOT NULL
   ,luck_per_move_normalised          FLOAT   NOT NULL
   ,luck_per_move                     FLOAT   NOT NULL
   ,luck_rating                       INTEGER NOT NULL
   -- cube statistics
   ,total_cube_decisions              INTEGER NOT NULL
   ,close_cube_decisions              INTEGER NOT NULL
   ,doubles                           INTEGER NOT NULL
   ,takes                             INTEGER NOT NULL
   ,passes                            INTEGER NOT NULL
   ,missed_doubles_below_cp           INTEGER NOT NULL
   ,missed_doubles_above_cp           INTEGER NOT NULL
   ,wrong_doubles_below_dp            INTEGER NOT NULL
   ,wrong_doubles_above_tg            INTEGER NOT NULL
   ,wrong_takes                       INTEGER NOT NULL
   ,wrong_passes                      INTEGER NOT NULL
   ,error_missed_doubles_below_cp_normalised     FLOAT   NOT NULL
   ,error_missed_doubles_above_cp_normalised     FLOAT   NOT NULL
   ,error_wrong_doubles_below_dp_normalised      FLOAT   NOT NULL
   ,error_wrong_doubles_above_tg_normalised      FLOAT   NOT NULL
   ,error_wrong_takes_normalised                 FLOAT   NOT NULL
   ,error_wrong_passes_normalised                FLOAT   NOT NULL
   ,error_missed_doubles_below_cp     FLOAT   NOT NULL
   ,error_missed_doubles_above_cp     FLOAT   NOT NULL
   ,error_wrong_doubles_below_dp      FLOAT   NOT NULL
   ,error_wrong_doubles_above_tg      FLOAT   NOT NULL
   ,error_wrong_takes                 FLOAT   NOT NULL
   ,error_wrong_passes                FLOAT   NOT NULL
   ,cube_error_total_normalised       FLOAT   NOT NULL
   ,cube_error_total                  FLOAT   NOT NULL
   ,cube_error_per_move_normalised    FLOAT   NOT NULL
   ,cube_error_per_move               FLOAT   NOT NULL
   ,cube_rating                       INTEGER NOT NULL
   -- overall statistics
   ,overall_error_total_normalised    FLOAT   NOT NULL
   ,overall_error_total               FLOAT   NOT NULL
   ,overall_error_per_move_normalised FLOAT   NOT NULL
   ,overall_error_per_move            FLOAT   NOT NULL
   ,overall_rating                    INTEGER NOT NULL
   ,actual_result                     FLOAT   NOT NULL
   ,luck_adjusted_result              FLOAT   NOT NULL
   ,snowie_moves                      INTEGER NOT NULL
   ,snowie_error_rate_per_move        FLOAT   NOT NULL
   -- for matches only
   ,luck_based_fibs_rating_diff       FLOAT           
   ,error_based_fibs_rating           FLOAT           
   ,chequer_rating_loss               FLOAT           
   ,cube_rating_loss                  FLOAT           
   -- for money sesisons only
   ,actual_advantage                  FLOAT           
   ,actual_advantage_ci               FLOAT           
   ,luck_adjusted_advantage           FLOAT           
   ,luck_adjusted_advantage_ci        FLOAT           
   -- time penalties
   ,time_penalties                    INTEGER NOT NULL
   ,time_penalty_loss_normalised      FLOAT   NOT NULL
   ,time_penalty_loss                 FLOAT   NOT NULL
   -- 
   ,PRIMARY KEY (matchstat_id)
   ,FOREIGN KEY (player_id) REFERENCES player (player_id)
      ON DELETE RESTRICT
   ,FOREIGN KEY (session_id) REFERENCES session (session_id)
      ON DELETE CASCADE
);

CREATE UNIQUE INDEX ismatchstat ON matchstat (
    matchstat_id
);


-- Table: game

CREATE TABLE game (
    game_id         INTEGER NOT NULL
   ,session_id        INTEGER NOT NULL
   -- Player 0
   ,player_id0      INTEGER NOT NULL 
   -- Player 1
   ,player_id1      INTEGER NOT NULL 
   -- score at start of game (since end of game is not meaningful if incomplete)
   ,score_0       INTEGER NOT NULL
   ,score_1       INTEGER NOT NULL
   -- The result of the game
   -- the total number of points won or lost
   -- positive means player 0 won game, negative player 1 won game, 
   -- zero - game not complete
   ,result          INTEGER NOT NULL
   -- Timestamp for insert into database
   ,added           TIMESTAMP NOT NULL
   -- which game in the match/session this is
   ,game_number     INTEGER NOT NULL
   -- if Crawford game
   ,crawford        INTEGER NOT NULL
   ,PRIMARY KEY (game_id)
   ,FOREIGN KEY (session_id) REFERENCES session (session_id)
      ON DELETE CASCADE
   ,FOREIGN KEY (player_id0) REFERENCES player (player_id)
      ON DELETE RESTRICT
   ,FOREIGN KEY (player_id1) REFERENCES player (player_id)
      ON DELETE RESTRICT
);

CREATE UNIQUE INDEX igame ON game (
    game_id
);

-- Table: statistics
-- Used from match and game tables to store match and game statistics,
-- respectively.

CREATE TABLE gamestat (
    gamestat_id                      INTEGER NOT NULL
   -- game identification
   ,game_id                          INTEGER NOT NULL
   -- player identification
   ,player_id                         INTEGER NOT NULL
   -- chequerplay statistics
   ,total_moves                       INTEGER NOT NULL 
   ,unforced_moves                    INTEGER NOT NULL
   ,unmarked_moves                    INTEGER NOT NULL
   ,good_moves                        INTEGER NOT NULL
   ,doubtful_moves                    INTEGER NOT NULL
   ,bad_moves                         INTEGER NOT NULL
   ,very_bad_moves                    INTEGER NOT NULL
   ,chequer_error_total_normalised    FLOAT   NOT NULL
   ,chequer_error_total               FLOAT   NOT NULL
   ,chequer_error_per_move_normalised FLOAT   NOT NULL
   ,chequer_error_per_move            FLOAT   NOT NULL
   ,chequer_rating                    INTEGER NOT NULL
   -- luck statistics
   ,very_lucky_rolls                  INTEGER NOT NULL
   ,lucky_rolls                       INTEGER NOT NULL
   ,unmarked_rolls                    INTEGER NOT NULL
   ,unlucky_rolls                     INTEGER NOT NULL
   ,very_unlucky_rolls                INTEGER NOT NULL
   ,luck_total_normalised             FLOAT   NOT NULL
   ,luck_total                        FLOAT   NOT NULL
   ,luck_per_move_normalised          FLOAT   NOT NULL
   ,luck_per_move                     FLOAT   NOT NULL
   ,luck_rating                       INTEGER NOT NULL
   -- cube statistics
   ,total_cube_decisions              INTEGER NOT NULL
   ,close_cube_decisions              INTEGER NOT NULL
   ,doubles                           INTEGER NOT NULL
   ,takes                             INTEGER NOT NULL
   ,passes                            INTEGER NOT NULL
   ,missed_doubles_below_cp           INTEGER NOT NULL
   ,missed_doubles_above_cp           INTEGER NOT NULL
   ,wrong_doubles_below_dp            INTEGER NOT NULL
   ,wrong_doubles_above_tg            INTEGER NOT NULL
   ,wrong_takes                       INTEGER NOT NULL
   ,wrong_passes                      INTEGER NOT NULL
   ,error_missed_doubles_below_cp_normalised     FLOAT   NOT NULL
   ,error_missed_doubles_above_cp_normalised     FLOAT   NOT NULL
   ,error_wrong_doubles_below_dp_normalised      FLOAT   NOT NULL
   ,error_wrong_doubles_above_tg_normalised      FLOAT   NOT NULL
   ,error_wrong_takes_normalised                 FLOAT   NOT NULL
   ,error_wrong_passes_normalised                FLOAT   NOT NULL
   ,error_missed_doubles_below_cp     FLOAT   NOT NULL
   ,error_missed_doubles_above_cp     FLOAT   NOT NULL
   ,error_wrong_doubles_below_dp      FLOAT   NOT NULL
   ,error_wrong_doubles_above_tg      FLOAT   NOT NULL
   ,error_wrong_takes                 FLOAT   NOT NULL
   ,error_wrong_passes                FLOAT   NOT NULL
   ,cube_error_total_normalised       FLOAT   NOT NULL
   ,cube_error_total                  FLOAT   NOT NULL
   ,cube_error_per_move_normalised    FLOAT   NOT NULL
   ,cube_error_per_move               FLOAT   NOT NULL
   ,cube_rating                       INTEGER NOT NULL
   -- overall statistics
   ,overall_error_total_normalised    FLOAT   NOT NULL
   ,overall_error_total               FLOAT   NOT NULL
   ,overall_error_per_move_normalised FLOAT   NOT NULL
   ,overall_error_per_move            FLOAT   NOT NULL
   ,overall_rating                    INTEGER NOT NULL
   ,actual_result                     FLOAT   NOT NULL
   ,luck_adjusted_result              FLOAT   NOT NULL
   ,snowie_moves                      INTEGER NOT NULL
   ,snowie_error_rate_per_move        FLOAT   NOT NULL
   -- for matches only
   ,luck_based_fibs_rating_diff       FLOAT           
   ,error_based_fibs_rating           FLOAT           
   ,chequer_rating_loss               FLOAT           
   ,cube_rating_loss                  FLOAT           
   -- for money sesisons only
   ,actual_advantage                  FLOAT           
   ,actual_advantage_ci               FLOAT           
   ,luck_adjusted_advantage           FLOAT           
   ,luck_adjusted_advantage_ci        FLOAT           
   -- time penalties
   ,time_penalties                    INTEGER NOT NULL
   ,time_penalty_loss_normalised      FLOAT   NOT NULL
   ,time_penalty_loss                 FLOAT   NOT NULL
   -- 
   ,PRIMARY KEY (gamestat_id)
   ,FOREIGN KEY (player_id) REFERENCES player (player_id)
      ON DELETE RESTRICT
   ,FOREIGN KEY (game_id) REFERENCES game (game_id)
      ON DELETE CASCADE
);

CREATE UNIQUE INDEX isgamestat ON gamestat (
    gamestat_id
);
