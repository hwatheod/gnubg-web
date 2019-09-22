// TODO: UI for moving checkers

checkerDiameter = 30;
gapBetweenCheckers = 5;
barLength = 40;
doublingCubeSize = 30;
doublingCubeOffset = 5;
dieSize = 30;
dieDotRadius = 3;
gapBetweenDice = 4;
maxCheckersShown = 5;
maxBarCheckersShown = 4;

resignFlagPoleLength = 36;
resignFlagSize = 20;

verticalGap = 50;
pointHeight = maxCheckersShown * checkerDiameter;

barLeftBoundary = checkerDiameter * 6 + gapBetweenCheckers * 7;
barRightBoundary = barLeftBoundary + barLength;
barCenter = (barLeftBoundary + barRightBoundary) / 2;
boardWidth = barRightBoundary + checkerDiameter * 6 + gapBetweenCheckers * 7;
boardHeight = maxCheckersShown * checkerDiameter * 2 + verticalGap;

playerLeftDieStartPoint = (barRightBoundary + boardWidth - gapBetweenDice) / 2 - dieSize;
playerRightDieStartPoint = (barRightBoundary + boardWidth + gapBetweenDice) / 2;
opponentLeftDieStartPoint = (barLeftBoundary - gapBetweenDice) / 2 - dieSize;
opponentRightDieStartPoint = (barLeftBoundary + gapBetweenDice) / 2;
diceVerticalStartPoint = (boardHeight - dieSize) / 2;

playerColor = "red";
opponentColor = "blue";

function drawCheckers(ctx, numCheckers, pointStart, direction) {
    if (numCheckers == 0) {
        return;
    }
     
    var checkerCenterVertical;
    if (direction == 1) { // top of board
	checkerCenterVertical = checkerDiameter / 2;
    } else {
	checkerCenterVertical = boardHeight - checkerDiameter / 2;
    }

    if (numCheckers > 0) {
        ctx.fillStyle = playerColor;
    } else {
        ctx.fillStyle = opponentColor;
    }

    for (var i=0; i<Math.min(Math.abs(numCheckers), maxCheckersShown); i++) {
        ctx.beginPath();
        ctx.arc(pointStart + checkerDiameter / 2, checkerCenterVertical, checkerDiameter / 2, 0, 2*Math.PI);
        ctx.fill();
        
        checkerCenterVertical += (direction * checkerDiameter);
    }

    if (Math.abs(numCheckers) > maxCheckersShown) {
        ctx.font = "14px sans-serif";
        ctx.fillStyle = "white";
        var text = Math.abs(numCheckers);
        ctx.fillText(text, pointStart + checkerDiameter / 2 - ctx.measureText(text).width / 2, checkerCenterVertical - direction * checkerDiameter + 2);
    }

    ctx.fillStyle = "grey";
}

function drawBarCheckers(ctx, numCheckers, direction) {
    if (numCheckers == 0) {
        return;
    }

    if (numCheckers > 0) {
        ctx.fillStyle = playerColor;
    } else {
        ctx.fillStyle = opponentColor;
    }

    var checkerCenterVertical = boardHeight / 2 + direction * (gapBetweenCheckers + checkerDiameter / 2);
    for (var i=0; i<Math.min(Math.abs(numCheckers), maxBarCheckersShown); i++) {
        ctx.beginPath();
        ctx.arc(barCenter, checkerCenterVertical, checkerDiameter / 2, 0, 2*Math.PI);
        ctx.fill();

        checkerCenterVertical += (direction * checkerDiameter);
    }

    if (Math.abs(numCheckers) > maxBarCheckersShown) {
        ctx.font = "14px sans-serif";
        ctx.fillStyle = "white";
        var text = Math.abs(numCheckers);
        ctx.fillText(Math.abs(numCheckers), barCenter - ctx.measureText(text).width/2, checkerCenterVertical - direction * checkerDiameter + 2);
    }

    ctx.fillStyle = "grey";
}

function drawBoard(backgroundOnly,
                   board,
		   matchLength,
		   myScore,
		   opponentScore,
		   turn,
		   dice1,
		   dice2,
		   cubeValue,
		   iMayDouble,
		   opponentMayDouble,
		   wasDoubled,
		   myPiecesOff,
		   opponentPiecesOff,
		   crawford,
                   resignationOffered,
		   resignationValue) {
    var backgammonBoard = document.getElementById("backgammonBoard");
    var ctx = backgammonBoard.getContext("2d");
    ctx.strokeStyle = "black";
    ctx.fillStyle = "grey";
    ctx.clearRect(0,0,backgammonBoard.width,backgammonBoard.height);

    // 1-12 = lower, 13-24 = upper
    // for now, we will draw it so that the player always plays counterclockwise
    // so the 1-point is in the lower left hand corner

    // draw outer boundary of board
    ctx.beginPath();
    ctx.rect(0,0,boardWidth,boardHeight);

    // draw bar
    ctx.moveTo(barLeftBoundary, 0);
    ctx.lineTo(barLeftBoundary, boardHeight);
    ctx.moveTo(barRightBoundary, 0);
    ctx.lineTo(barRightBoundary, boardHeight);
      
    ctx.stroke();

    // draw upper left points
    var pointStart = gapBetweenCheckers;
    for (var i=0; i<6; i++) {
        ctx.beginPath();
        ctx.moveTo(pointStart, 0);
        ctx.lineTo(pointStart + checkerDiameter / 2, pointHeight);
        ctx.lineTo(pointStart + checkerDiameter, 0);
        if (i % 2 == 0) {
	    ctx.stroke();
        } else {
	    ctx.fill();
        }
	if (!backgroundOnly) {
	    drawCheckers(ctx, board[24-i], pointStart, 1);
	}

        pointStart += (checkerDiameter + gapBetweenCheckers);
    }

    // draw upper right points
    pointStart += (barLength + gapBetweenCheckers);
    for (var i=0; i<6; i++) {
        ctx.beginPath();
        ctx.moveTo(pointStart, 0);
        ctx.lineTo(pointStart + checkerDiameter / 2, pointHeight);
        ctx.lineTo(pointStart + checkerDiameter, 0);
        if (i % 2 == 0) {
	    ctx.stroke();
        } else {
	    ctx.fill();
        }
	if (!backgroundOnly) {
	    drawCheckers(ctx, board[18-i], pointStart, 1);
	}

        pointStart += (checkerDiameter + gapBetweenCheckers);
    }


    // draw lower left points
    var pointStart = gapBetweenCheckers;
    for (var i=0; i<6; i++) {
        ctx.beginPath();
        ctx.moveTo(pointStart, boardHeight);
        ctx.lineTo(pointStart + checkerDiameter / 2, boardHeight - pointHeight);
        ctx.lineTo(pointStart + checkerDiameter, boardHeight);
        if (i % 2 == 1) {
	    ctx.stroke();
        } else {
	    ctx.fill();
        }
	if (!backgroundOnly) {
	    drawCheckers(ctx, board[i+1], pointStart, -1);
	}
        pointStart += (checkerDiameter + gapBetweenCheckers);
    }

    // draw upper right points
    pointStart += (barLength + gapBetweenCheckers);
    for (var i=0; i<6; i++) {
        ctx.beginPath();
        ctx.moveTo(pointStart, boardHeight);
        ctx.lineTo(pointStart + checkerDiameter / 2, boardHeight - pointHeight);
        ctx.lineTo(pointStart + checkerDiameter, boardHeight);
	if (i % 2 == 1) {
	    ctx.stroke();
        } else {
	    ctx.fill();
        }
        if (!backgroundOnly) {
	    drawCheckers(ctx, board[i+7], pointStart, -1);
	}

        pointStart += (checkerDiameter + gapBetweenCheckers);
    }

    if (backgroundOnly) {
	return;
    }

    // draw bar checkers
    // my bar checkers start slightly above the center of the bar and each successive one goes up
    // opponent's bar checkers start slightly below the center of the bar and each successive one goes down
    drawBarCheckers(ctx, board[25], -1);
    drawBarCheckers(ctx, board[0], 1);

    // draw dice
    if (dice1 > 0) {
	drawDice(ctx, dice1, dice2, turn);
    }

    if (!crawford) {
        ctx.strokeStyle = "black";
        var cubeVertical;
	var cubeHorizontal;
	var cubeValueToShow;
        if (wasDoubled) {
	    cubeValueToShow = cubeValue * 2;
	    cubeVertical = (boardHeight - doublingCubeSize) / 2;
	    if (wasDoubled > 0) {  // opponent doubled player
		cubeHorizontal = (barLeftBoundary - doublingCubeSize) / 2;
	    } else {  // player doubled opponent
		cubeHorizontal = (barRightBoundary + boardWidth - doublingCubeSize) / 2;
	    }
	} else {
	    cubeValueToShow = cubeValue;
	    cubeHorizontal = boardWidth + doublingCubeOffset;
	    if (iMayDouble && opponentMayDouble) { // centered cube
		cubeVertical = (boardHeight - doublingCubeSize) / 2;
	    } else if (iMayDouble) {
		cubeVertical = boardHeight - doublingCubeSize;
	    } else if (opponentMayDouble) {
		cubeVertical = 2;
	    }
	}
        ctx.strokeRect(cubeHorizontal, cubeVertical, doublingCubeSize, doublingCubeSize);
        ctx.font = "14px sans-serif";
        ctx.fillStyle = "black";
        ctx.fillText(cubeValueToShow, cubeHorizontal + (doublingCubeSize - ctx.measureText(cubeValueToShow).width) / 2, cubeVertical + doublingCubeSize/2 + 4);
    }

    if (myPiecesOff > 0) {
	ctx.fillStyle = playerColor;
	var checkerOffHorizontal = boardWidth + doublingCubeOffset + doublingCubeSize / 2;
	var checkerOffVertical = boardHeight - doublingCubeSize - gapBetweenCheckers - checkerDiameter / 2;
	ctx.beginPath();
	ctx.arc(checkerOffHorizontal, checkerOffVertical, checkerDiameter / 2, 0, 2*Math.PI);
	ctx.fill();
	ctx.fillStyle = "white";
	ctx.font = "14px sans-serif";
	ctx.fillText(myPiecesOff, checkerOffHorizontal - ctx.measureText(myPiecesOff).width/2, checkerOffVertical + 4);
    }

    if (opponentPiecesOff > 0) {
	ctx.fillStyle = opponentColor;
	var checkerOffHorizontal = boardWidth + doublingCubeOffset + doublingCubeSize / 2;
	var checkerOffVertical = 2 + doublingCubeSize + gapBetweenCheckers + checkerDiameter / 2;
	ctx.beginPath();
	ctx.arc(checkerOffHorizontal, checkerOffVertical, checkerDiameter / 2, 0, 2*Math.PI);
	ctx.fill();
	ctx.fillStyle = "white";
	ctx.font = "14px sans-serif";
	ctx.fillText(opponentPiecesOff, checkerOffHorizontal - ctx.measureText(opponentPiecesOff).width/2, checkerOffVertical + 4);
    }

    if (resignationOffered) {
	var resignationFlagHorizontal;
	if (turn == 1) {  // player offered resignation to opponent
	    resignationFlagHorizontal = barLeftBoundary / 2;
	} else {  // opponent offered resignation to player
	    resignationFlagHorizontal = (barRightBoundary + boardWidth)/2;
	}

	ctx.beginPath();
	ctx.strokeStyle = "black";
	ctx.moveTo(resignationFlagHorizontal, (boardHeight - resignFlagPoleLength)/2);
	ctx.lineTo(resignationFlagHorizontal, (boardHeight + resignFlagPoleLength)/2);
	ctx.stroke();
	ctx.strokeRect(resignationFlagHorizontal, (boardHeight - resignFlagPoleLength)/2, resignFlagSize, resignFlagSize);
	ctx.fillStyle = "black";
	ctx.font = "14px sans-serif";
	ctx.fillText(resignationValue, resignationFlagHorizontal + resignFlagSize / 2 - ctx.measureText(resignationValue).width / 2,  (boardHeight - resignFlagPoleLength + resignFlagSize)/2 + 4);
    }

    var info = document.getElementById("info");
    info.innerHTML = "Score: " + myScore + "-" + opponentScore + (matchLength > 0 ? " Match to: " + matchLength : "") + (crawford ? " Crawford" : "");
    var instructions = document.getElementById("instructions");
    if (turn == 0) {
	instructions.innerHTML = "";
    } else {
	if (dice1 > 0) {
	    instructions.innerHTML = "Move checkers";
	    document.getElementById("roll").disabled = true;
	    document.getElementById("double").disabled = true;
	    document.getElementById("accept").disabled = true;
	    document.getElementById("reject").disabled = true;
	    document.getElementById("beaver").disabled = true;
	    document.getElementById("resign").disabled = false;
	} else if (wasDoubled) {
	    instructions.innerHTML = "Accept or reject the double";
	    document.getElementById("roll").disabled = true;
	    document.getElementById("double").disabled = true;
	    document.getElementById("accept").disabled = false;
	    document.getElementById("reject").disabled = false;
	    document.getElementById("beaver").disabled = (matchLength > 0);
	    document.getElementById("resign").disabled = true;
        } else if (resignationOffered) {
	    instructions.innerHTML = "Accept or reject the resignation";
	    document.getElementById("roll").disabled = true;
	    document.getElementById("double").disabled = true;
	    document.getElementById("accept").disabled = false;
	    document.getElementById("reject").disabled = false;
	    document.getElementById("beaver").disabled = true;
	    document.getElementById("resign").disabled = true;
	} else {
	    instructions.innerHTML = "Roll or double";
	    document.getElementById("roll").disabled = false;
	    document.getElementById("double").disabled = false;
	    document.getElementById("accept").disabled = true;
	    document.getElementById("reject").disabled = true;
	    document.getElementById("beaver").disabled = true;
	    document.getElementById("resign").disabled = false;
	}
    }
}

function intermediatePoint(a, b, t) {
    return (1-t)*a + b*t;
}
/*
  -----
 |     |
 |  .  |
 |     |
  -----     
 */
function drawDieCenterDot(ctx, dieStartPoint) {
    ctx.fillStyle = "white";
    ctx.beginPath();
    ctx.arc(
	    intermediatePoint(dieStartPoint, dieStartPoint + dieSize, 0.5),
	    intermediatePoint(diceVerticalStartPoint, diceVerticalStartPoint + dieSize, 0.5),
	    dieDotRadius, 0, 2*Math.PI);
    ctx.fill();
}

/*
  -----
 | .   |
 |     |
 |    .|
  -----     
 */
function drawDieMainDiagonalDots(ctx, dieStartPoint) {
    ctx.fillStyle = "white";
    ctx.beginPath();
    ctx.arc(
	    intermediatePoint(dieStartPoint, dieStartPoint + dieSize, 0.25),
	    intermediatePoint(diceVerticalStartPoint, diceVerticalStartPoint + dieSize, 0.25),
	    dieDotRadius, 0, 2*Math.PI);
    ctx.arc(
	    intermediatePoint(dieStartPoint, dieStartPoint + dieSize, 0.75),
	    intermediatePoint(diceVerticalStartPoint, diceVerticalStartPoint + dieSize, 0.75),
	    dieDotRadius, 0, 2*Math.PI);
    ctx.fill();
}

/*
  -----
 |    .|
 |     |
 | .   |
  -----     
 */
function drawDieAntiDiagonalDots(ctx, dieStartPoint) {
    ctx.fillStyle = "white";
    ctx.beginPath();
    ctx.arc(
	    intermediatePoint(dieStartPoint, dieStartPoint + dieSize, 0.75),
	    intermediatePoint(diceVerticalStartPoint, diceVerticalStartPoint + dieSize, 0.25),
	    dieDotRadius, 0, 2*Math.PI);
    ctx.arc(
	    intermediatePoint(dieStartPoint, dieStartPoint + dieSize, 0.25),
	    intermediatePoint(diceVerticalStartPoint, diceVerticalStartPoint + dieSize, 0.75),
	    dieDotRadius, 0, 2*Math.PI);
    ctx.fill();
}

/*
  -----
 |     |
 | .  .|
 |     |
  -----     
 */
function drawDieMiddleDots(ctx, dieStartPoint) {
    ctx.fillStyle = "white";
    ctx.beginPath();
    ctx.arc(
	    intermediatePoint(dieStartPoint, dieStartPoint + dieSize, 0.25),
	    intermediatePoint(diceVerticalStartPoint, diceVerticalStartPoint + dieSize, 0.5),
	    dieDotRadius, 0, 2*Math.PI);
    ctx.arc(
	    intermediatePoint(dieStartPoint, dieStartPoint + dieSize, 0.75),
	    intermediatePoint(diceVerticalStartPoint, diceVerticalStartPoint + dieSize, 0.5),
	    dieDotRadius, 0, 2*Math.PI);
    ctx.fill();
}


function drawDie(ctx, dieStartPoint, color, dieValue) {
    ctx.fillStyle = color;
    ctx.fillRect(dieStartPoint, diceVerticalStartPoint, dieSize, dieSize);
    switch (dieValue) {
       case 1:
	   drawDieCenterDot(ctx, dieStartPoint);
           break;
       case 2:
           drawDieMainDiagonalDots(ctx, dieStartPoint);
           break;
       case 3:
           drawDieMainDiagonalDots(ctx, dieStartPoint);
           drawDieCenterDot(ctx, dieStartPoint);
           break;
       case 4:
           drawDieMainDiagonalDots(ctx, dieStartPoint);
           drawDieAntiDiagonalDots(ctx, dieStartPoint);
           break;
       case 5:
           drawDieMainDiagonalDots(ctx, dieStartPoint);
           drawDieAntiDiagonalDots(ctx, dieStartPoint);
	   drawDieCenterDot(ctx, dieStartPoint);
	   break;
       case 6:
           drawDieMainDiagonalDots(ctx, dieStartPoint);
           drawDieAntiDiagonalDots(ctx, dieStartPoint);
           drawDieMiddleDots(ctx, dieStartPoint);
           break;

       default:
           break;
    } 

}

function drawDice(ctx, n1, n2, turn) {
    var color, leftDieStartPoint, rightDieStartPoint;
    if (turn == 1) {
	color = playerColor;
	leftDieStartPoint = playerLeftDieStartPoint;
	rightDieStartPoint = playerRightDieStartPoint;
    } else {
	color = opponentColor;
	leftDieStartPoint = opponentLeftDieStartPoint;
	rightDieStartPoint = opponentRightDieStartPoint;
    }

    drawDie(ctx, leftDieStartPoint, color, n1);
    drawDie(ctx, rightDieStartPoint, color, n2);
}