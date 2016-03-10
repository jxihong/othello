#include "player.h"

bitset<64> EDGES = bitset<64>(0xff818181818181ffULL);
bitset<64> CORNERS = bitset<64>(0x8100000000000081ULL);

/*
 * Constructor for the player; initialize everything here. The side your AI is
 * on (BLACK or WHITE) is passed in as "side". The constructor must finish 
 * within 30 seconds.
 */
Player::Player(Side side) : _side(side) {
    // Will be set to true in test_minimax.cpp.
    testingMinimax = false;
    
    /* 
     * TODO: Do any initialization you need to do here (setting up the board,
     * precalculating things, etc.) However, remember that you will only have
     * 30 seconds.
     */
    _board = new Board();
    _opponentSide = (_side == BLACK) ? (WHITE) : (BLACK);

    this->computeOpening();
    std::cerr << "Done Initialization" << std::endl;
}

/*
 * Destructor for the player.
 */
Player::~Player() {
    _table.clear();
    delete _board;
}

/*
 * Compute the next move given the opponent's last move. Your AI is
 * expected to keep track of the board on its own. If this is the first move,
 * or if the opponent passed on the last move, then opponentsMove will be NULL.
 *
 * msLeft represents the time your AI has left for the total game, in
 * milliseconds. doMove() must take no longer than msLeft, or your AI will
 * be disqualified! An msLeft value of -1 indicates no time limit.
 *
 * The move returned must be legal; if there are no valid moves for your side,
 * return NULL.
 */
Move *Player::doMove(Move *opponentsMove, int msLeft) {
    /* 
     * TODO: Implement how moves your AI should play here. You should first
     * process the opponent's opponents move before calculating your own move
     */ 
    if (opponentsMove) {
	_board->doMove(opponentsMove, _opponentSide);
    }
    Move *m = NULL;
    // If a time limit is specified, the AI will do iterative deepening to 
    // use up as much time as safely possible.
    if (msLeft > 0) {
	clock_t start = clock();

	// 500 may be an overestimate. Can optimize later
	double time_allowed = (msLeft) / (500);
	
	int depth = MINIMAXDEPTH;

	// While there is still time left, it will compute one depth further. While
	// it repeats some calculations, that fact that we have a transposition table
	// should minimize the time wasted. 
	while ( (double)(clock() - start) / (CLOCKS_PER_SEC / 1000) < time_allowed) {
	    m = (testingMinimax) ? 
		(this->findMinimaxMove(2)) : (this->findMinimaxMove(depth++));
	}
	/* FOR DEBUGGING
	cerr << depth << endl;
	*/
    }
    else {
	m = (testingMinimax) ? 
	    (this->findMinimaxMove(2)) : (this->findMinimaxMove(MINIMAXDEPTH));
    }
    _board->doMove(m, _side);
    return m;
}

/*
 * Returns the first available move that the AI finds.
 */
Move *Player::findFirstMove() {
    vector<Move *> moves;
    _board->getPossibleMoves(_side, moves);
    
    // Returns the first move in the list
    if (!moves.empty()) {
	return moves.front();
    }
    
    // No valid moves
    return NULL;
}

/*
 * Uses the minimax algorithm to look for the best move
 */
Move *Player::findMinimaxMove(int depth) {
    int alpha = INT_MIN;
    int beta = INT_MAX;
    
    vector<Move *> moves;
    _board->getPossibleMoves(_side, moves);
    
    Move *best = NULL; // Stores best move
    
    for (vector<Move *>::iterator it = moves.begin(); it != moves.end(); ++it) {
	Board *nextBoard = _board->copy();
	nextBoard->doMove(*it, _side);
	
	// Calls helper function that returns best score given move *it 
	int score = this->minimaxHelper(depth - 1, nextBoard, _opponentSide, alpha, beta);
	if (score >= alpha) {
	    alpha = score;
	    best = *it;
	}
	if (*it != best) {
	    delete *it;
	}
	delete nextBoard;
    }
    
    /* FOR THREADING
    vector<future<int> > futures;
    
    vector<Board *> boards(moves.size(), _board->copy());
    int i = 0;
    for (vector<Move *>::iterator it = moves.begin(); it != moves.end(); ++it) {
	boards[i]->doMove(*it, _side);
	
	futures.push_back(async(&Player::minimaxHelper, this, depth - 1, boards[i++],
				_opponentSide, alpha, beta));
    }
    
    for (vector<future<int> >::iterator it = futures.begin(); it != futures.end(); ++it) {
	cerr <<  "Score: " << it->get() << endl;
    }
    */
    
    return best;
}

/*
 * Helper function that recursively searches for the minimax
 * solution. Returns a pair including the optimized score alpha/beta
 * and the best move.
 */
int Player::minimaxHelper(int depth, Board *b, Side s, int alpha, int beta) {
    // Base Case: Just evaluate board
    if (depth == 0) {
	return this->evaluate(b);
    }
    else {
	vector<Move *> moves;
	b->getPossibleMoves(s, moves);

	// There are no more possible moves
	if (moves.empty()) {
	    return this->evaluate(b);
	}
	if (s == _side) {
	    alpha = INT_MIN;
	    for (vector<Move *>::iterator it = moves.begin(); it != moves.end(); ++it) {
		Board *nextBoard = b->copy();
		nextBoard->doMove(*it, s);
		
		// Wants to maximize the possible score
		int score = this->minimaxHelper(depth - 1, nextBoard, _opponentSide, alpha, beta);
		alpha = max(alpha, score);
		
		if (beta <= alpha) {
		    break;
		}
		delete *it;
		delete nextBoard;
	    }
	    return alpha;
	}
	else {
	    beta = INT_MAX;
	    for (vector<Move *>::iterator it = moves.begin(); it != moves.end(); ++it) {
		Board *nextBoard = b->copy();
		nextBoard->doMove(*it, s);
		
		// Opponent wants to minimize the possible score
		int score = this->minimaxHelper(depth - 1, nextBoard, _side, alpha, beta);
		beta = min(beta, score);
		
		if (beta <= alpha) {
		    break;
		}
		delete *it;
		delete nextBoard;
	    }
	    return beta;
	}
    }
}

/*
 * Heuristic that evaluates the score of a given board
 * configuration.
 */ 
int Player::evaluate(Board *b) {
    if (testingMinimax) {
	return b->count(_side) - b->count(_opponentSide);
    }
    else {
	// Compute the bitsets for ai and opponent sides, and converts
	// to a 64-bit integer
	unsigned long long ai_side, total;
	if (_side == BLACK) {
	    ai_side = (b->black).to_ullong();
	    total = (b->taken).to_ullong();
	}
	else {
	    ai_side = (~(b->black) & (b->taken)).to_ullong();
	    total = (b->taken).to_ullong();
	}
	
	// Hash value is simply concatenation of 2 integers
	string hash = to_string(ai_side) + ", " + to_string(total);
	if (_table.find(hash) != _table.end()) {
	    return _table[hash];
	}
	else {
	    int score = 0;
        bitset<64> white = (b->taken & ~(b->black));
        // Coin count
        score += (b->black).count() - white.count();
        
        // Edges and corners
        score += EDGEWEIGHT * (b->black & EDGES).count();
        score += CORNERWEIGHT * (b->black & CORNERS).count();
        score -= EDGEWEIGHT * (white & EDGES).count();
        score -= CORNERWEIGHT * (white & CORNERS).count();
        
        // Mobility
        vector<Move *> next_moves;
        b->getPossibleMoves(BLACK, next_moves);
        Move *to_delete;
        while (!next_moves.empty()) {
            to_delete = next_moves.back();
            next_moves.pop_back();
            delete to_delete;
        }
//        score += next_moves.size();
//        next_moves.clear();
//        b->getPossibleMoves(WHITE, next_moves);
//        score -= next_moves.size();
        
        if (_side == WHITE) {
            score *= -1;
        }
	    
	    // Keeps transposition table at fixed size
	    if (_table.size() >= 100000) {
		// Removes the first key, since it is probably the furthest 
		// away from the current game state
		_table.erase(_table.begin());
	    }
	    _table[hash] = score;
	    return score;
	} 
    }
}

/*
 * Stores the score of positions early in the game so they can be 
 * looked up quickly.
 */
void Player::computeOpening() {
    vector<pair<Side, Board *> > positions;
    positions.push_back(make_pair(_side, _board->copy()));
    
    // Computes initial 25000 board positions, and stores their 
    // score in the transposition table
    while (_table.size() < 25000) {
	pair<Side, Board *> curr = positions.front();
	this->evaluate(curr.second);
	
	positions.erase(positions.begin());

	vector<Move *> moves;
	curr.second->getPossibleMoves(curr.first, moves);
    Side next = (curr.first == BLACK) ? (WHITE) : (BLACK);

	
	for (vector<Move *>::iterator it = moves.begin(); it != moves.end(); ++it) {
	    Board *nextBoard = curr.second->copy();
	    nextBoard->doMove(*it, curr.first);
	    
	    positions.push_back(make_pair(next, nextBoard));
	}
	
	delete curr.second;
    }
    
    while (!positions.empty()) {
	pair<Side, Board *> it = positions.back();
	positions.pop_back();
	delete it.second;
    }
	
}

