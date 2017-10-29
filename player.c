//A AI to player the final card down game
//verison 1
//2017-10-24
//Yingzhi Zhou (z5125679)

#include "Game.h"
#include "player.h"
#include "Card.h"
#include <stdio.h>
#include <stdlib.h>
#define NOT_FOUND -1

#define callOutTRIO 3
#define callOutDUO 2
#define callOutUNO 1

static int callOut(Game game);
static int noMatchingCard (Game game);
static int haveMatchingCard (Game game);
static int shouldSayUNO (Game game);
static int shouldSayDUO (Game game);
static int shouldSayTRIO (Game game);
static int haveDrawnCard(Game game);
static int havePlayedCard(Game game);
static int canPlayCard (Game game);
static Card decideCard (Game game);
static Card decideFirstCard (Game game);

playerMove decideMove(Game game) {
    playerMove move;
    move.action = END_TURN;
    int flag = 0;

    // Start out by making a move struct, to say what our move is.
    // If the player is the first player, do not call out
    // Otherwise, Check if the last player said UNO, DUO or TRIO
    if (numTurns(game) != 1) {
        int callOutValue = callOut(game);
        if (callOutValue == callOutUNO) {
            move.action = SAY_UNO;
            flag = 1;
        } else if (callOutValue == callOutDUO) {
            move.action = SAY_DUO;
            flag = 1;
        } else if (callOutValue == callOutTRIO) {
            move.action = SAY_TRIO;
            flag = 1;
        }
    }

    // The first move in a game must be PLAY_CARD
    if (numTurns(game) == 1 && turnMoves(game, currentTurn(game)) == 0) {
        move.action = PLAY_CARD;
        move.card = decideFirstCard(game);
        flag = 1;
    }

    // If the player can draw the card, the action of this move is draw card
    // Determine whether the player can currently draw a card.
    // If they can't draw a card, they should probably end their turn.
    if (flag == 0 && noMatchingCard(game)) {
        // DRAW_CARD --> END_TURN
        if (turnMoves(game, currentTurn(game)) == 0) {
            move.action = DRAW_CARD;
            flag = 1;
        } else if (turnMoves(game, currentTurn(game)) == 1) {
            playerMove lastMove = pastMove(game, currentTurn(game), 0);
            // CALL OUT --> DRAW_CARD
            if (lastMove.action == SAY_UNO
                || lastMove.action == SAY_DUO
                || lastMove.action == SAY_TRIO) {
                move.action = DRAW_CARD;
                flag = 1;
            // DRAW_CARD --> DRAW_CARD
            } else if (lastMove.action == DRAW_CARD
                && cardValue(topDiscard(game) == DRAW_TWO)) {
                move.action = DRAW_CARD;
                flag = 1;
            }
        } else if (turnMoves(game, currentTurn(game)) == 2) {
            // CALL OUT --> DRAW_CARD --> DRAW_CARD
            if (cardValue(topDiscard(game)) == DRAW_TWO) {
                playerMove firstMove = pastMove(game, currentTurn(game), 0);
                playerMove secondMove = pastMove(game, currentTurn(game), 1);
                if ((firstMove.action == SAY_UNO
                    || firstMove.action == SAY_DUO
                    || firstMove.action == SAY_TRIO)
                    && secondMove.action == DRAW_CARD
                    && cardValue(topDiscard(game)) == DRAW_TWO){
                    move.action = DRAW_CARD;
                    flag = 1;
                }
            }
        }
    }

    // Claim card
    if (flag == 0 && shouldSayTRIO(game)) {
        move.action = SAY_TRIO;
        flag = 1;
    }
    if (flag == 0 && shouldSayDUO(game)) {
        move.action = SAY_DUO;
        flag = 1;
    }
    if (flag == 0 && shouldSayUNO(game)) {
        move.action = SAY_UNO;
        flag = 1;
    }

    // If the player has valid cards to play,
    // Choose one
    if (flag == 0 && canPlayCard(game)) {
        move.action = PLAY_CARD;
        move.card = decideCard(game);
        flag = 1;
    }

    return move;
}

/* *****************************************************************************
 * Helper function
 * ***************************************************************************** */

// If the last player called out, return FALSE;
// Otherwise, check the number of the cards of the last player
static int callOut(Game game) {
    int callOutValue = 0;

    int turnIndex = currentTurn(game) - 1;
    int moveIndex = turnMoves(game, turnIndex) - 2;
    playerMove secondLastMove = pastMove(game, lastTurn, moveIndex);
    if (secondLastMove.action != SAY_UNO
        && secondLastMove.action != SAY_DUO
        && secondLastMove.action != SAY_TRIO) {
        // Find the lastPlayer
        int lastPlayer;
        if (playDirection (game) == CLOCKWISE) {
            if (secondLastMove.action == PLAY_CARD
                && cardValue(topDiscard(game)) == ADVANCE) {
                lastPlayer = (currentPlayer (game) - 2) % 4;
            } else {
                lastPlayer = (currentPlayer (game) - 1) % 4;
            }
        } else {
            if (secondLastMove.action == PLAY_CARD
                && cardValue(topDiscard(game)) == ADVANCE) {
                lastPlayer = (currentPlayer (game) + 2) % 4;
            } else {
                lastPlayer = (currentPlayer (game) + 1) % 4;
            }
        }
        // Check the number of cards in the last player
        if (playerCardCount(game, lastPlayer) == 1) {
            callOutValue = callOutUNO;
        } else if (playerCardCount(game, lastPlayer) == 2) {
            callOutValue = callOutDUO;
        } else if (playerCardCount(game, lastPlayer) == 3) {
            callOutValue = callOutTRIO;
        }
    }
    return callOutValue;
}


// Check whether the player does not have valid cards
static int noMatchingCard (Game game) {
    int noMatching = TRUE;

    value suit = cardSuit(topDiscard(game));
    value color = cardColor(topDiscard(game));
    value value = cardValue(topDiscard(game));

    int i = 0;
    Card curr = NULL;
    while (i < playerCardCount(game, currentPlayer(game))) {
        curr = handCard(game, i);
        if (cardValue(curr) == suit
            || cardValue(curr) == color
            || cardValue(curr) == value) {
            noMatching = FALSE;
            break;
        }
        i++;
    }
    return noMatching;
}

// Check whether the player has valid cards
static int haveMatchingCard (Game game) {
    int matching = TRUE;
    if (noMatchingCard(game)) {
        matching = FALSE;
    }
    return matching;
}

// Determine whether the current player should SAY_UNO.
// There are two different situations where it could be a
// valid move to SAY_UNO.
// For now, just deal with the simple situation: "claim card".
// Note: there are several possible ways to determine this.
static int shouldSayUNO (Game game) {
    //if current player has 1 card left in hand
    int shouldSay = FALSE;
    if (handCardCount(game) == 1) {
        shouldSay = TRUE;
    }
    //if the last player forget to say uno;
    return shouldSay;
}

// Determine whether the current player should SAY_UNO.
// There are two different situations where it could be a
// valid move to SAY_DUO.
// For now, just deal with the simple situation: "claim card".
// Note: there are several possible ways to determine this.
static int shouldSayDUO (Game game) {
    //if current player has 2 card left in hand
    int shouldSay = FALSE;
    if (handCardCount(game) == 2) {
        shouldSay = TRUE;
    }
    //if the last player forget to say duo;
    return shouldSay;
}

// Determine whether the current player should SAY_UNO.
// There are two different situations where it could be a
// valid move to SAY_TRIO.
// For now, just deal with the simple situation: "claim card".
// Note: there are several possible ways to determine this.
static int shouldSayTRIO (Game game) {
    //if current player has 3 card left in hand
    int shouldSay = FALSE;
    if (handCardCount(game) == 3) {
        shouldSay = TRUE;
    }
    //if the last player forget to say trio;
    return shouldSay;
}

// Check whether the current player has drawn a card in the current turn
static int haveDrawnCard(Game game) {
    int drawn = FALSE;
    int numOfMoves = turnMoves(game, currentTurn(game));
    if (numOfMoves != 0) {
        int i = 0;
        playerMove currMove;
        while (i < numOfMoves) {
            currMove = pastMove(game, currentTurn(game), i);
            if (currMove.action == DRAW_CARD) {
                drawn = TRUE;
                break;
            }
        }
    }
    return drawn;
}

// Check whether the current player has played a card in the current turn
static int havePlayedCard(Game game) {
    int played = FALSE;
    int numOfMoves = turnMoves(game, currentTurn(game));
    if (numOfMoves != 0) {
        int i = 0;
        playerMove currMove;
        while (i < numOfMoves) {
            currMove = pastMove(game, currentTurn(game), i);
            if (currMove.action == PLAY_CARD) {
                played = TRUE;
                break;
            }
        }
    }
    return played;
}

// Check whether the current player can play a card
static int canPlayCard (Game game) {
    int canPlay = TRUE;
    if (haveDrawnCard(game)) {
        canPlay = FALSE;
    } else {
        if (havePlayedCard(game) && cardValue(topDiscard(game)) != CONTINUE) {
            canPlay = FALSE;
        } else if (noMatchingCard(game)) {
            canPlay = FALSE;
        }
    }
    return canPlay;
}

// Decide the best card to play
// CONTINUE -> DRAW_TWO -> the larger value
static Card decideCard (Game game) {
    value suit = cardSuit(topDiscard(game));
    value color = cardColor(topDiscard(game));
    value value = cardValue(topDiscard(game));

    int index = -1;
    int max = -1;

    int i = 0;
    Card curr = NULL;
    while (i < playerCardCount(game, currentPlayer(game))) {
        curr = handCard(game, i);
        if (cardValue(curr) == suit
            || cardValue(curr) == color
            || cardValue(curr) == value) {
            if (cardValue(curr) == CONTINUE) {
                index = i;
                break;
            }
            if (cardValue(curr) == DRAW_TWO) {
                index = i;
                break;
            }
            if (cardValue(curr) >= max) {
                index = i;
            }
        }
        i++;
    }
    return handCard(game, index);
}

// Decide the best card to play in the first in a game
// CONTINUE -> DRAW_TWO -> the larger value
static Card decideFirstCard (Game game) {
    int index = -1;
    int max = -1;

    int i = 0;
    Card curr = NULL;
    while (i < playerCardCount(game, currentPlayer(game))) {
        curr = handCard(game, i);
        if (cardValue(curr) == CONTINUE) {
            index = i;
            break;
        }
        if (cardValue(curr) == DRAW_TWO) {
            index = i;
            break;
        }
        if (cardValue(curr) >= max) {
            index = i;
        }
        i++;
    }
    return handCard(game, index);
}
