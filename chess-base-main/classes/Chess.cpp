#include "Chess.h"
#include <limits>
#include <cmath>
#include <iostream> // Added for debug output

// Initialize static members
BitBoard Chess::_knightBitboards[64];
bool Chess::_knightBitboardsInitialized = false;

Chess::Chess()
{
    _grid = new Grid(8, 8);
    _selectedPiece = nullptr;
    _selectedPieceSource = nullptr;
}

Chess::~Chess()
{
    delete _grid;
    // Note: _selectedPiece and _selectedPieceSource are just pointers, don't delete
    _selectedPiece = nullptr;
    _selectedPieceSource = nullptr;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);
    
    // Set gameTag: piece type (1-6) + 128 for black pieces
    int gameTag = (int)piece;
    if (playerNumber == 1) {  // Black
        gameTag += 128;
    }
    bit->setGameTag(gameTag);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // Clear board - > nullptr
    // row 7, col 0
    // change col per /,
    
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    //we want to loop through each char in fen and update on the board
    // for now lets print first char fen[0]
    // FEN FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    auto getType = [](char c) -> int{
            switch (c)
            {
                case 'p' : case 'P':
                return 1;
                case 'n': case 'N':
                return 2;
                case 'b': case 'B':
                return 3;
                case 'r': case 'R':
                return 4;
                case 'q': case 'Q':
                return 5;
                case 'k': case 'K':
                return 6;
            }
        }; // 1 pawn, 2 Knignt, 3 Bishop, 4 Rook, 5 Queen, 6 King

    int row = 7;
    int col = 0;
    for (char c : fen)
    {
        // if / we row-- and we col 0
        if(c == '/')
        {
            row--;
            col = 0;
        }
        else if(isdigit(c))
        {
            col += (int)c;
        }
        else
        {
            int playerType = getType(c);
            int playerColor = isupper(c) ? 0 : 1; // white = 0, black = 1
            Bit* currBit = PieceForPlayer(playerColor, ChessPiece(playerType));
            BitHolder* holder = _grid->getSquare(col, row);
            holder->setBit(currBit);
            currBit->setPosition(holder->getPosition());
            col++; // at the end we move to next column
        }
    }

    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    // Check if we have a selected piece and this is a valid destination
    if (_selectedPiece && _selectedPieceSource) {
        ChessSquare* targetSquare = dynamic_cast<ChessSquare*>(&holder);
        if (targetSquare && targetSquare->highlighted()) {
            // Validate the move
            if (canBitMoveFromTo(*_selectedPiece, *_selectedPieceSource, *targetSquare)) {
                // Clear highlights first
                clearBoardHighlights();
                
                // Handle capture if there's a piece on the target square
                if (targetSquare->bit()) {
                    pieceTaken(targetSquare->bit());
                }
                
                // Move the piece
                if (targetSquare->dropBitAtPoint(_selectedPiece, targetSquare->getPosition())) {
                    // Update piece position
                    _selectedPiece->setPosition(targetSquare->getPosition());
                    
                    // Clear the source square
                    if (_selectedPieceSource) {
                        _selectedPieceSource->draggedBitTo(_selectedPiece, targetSquare);
                    }
                    
                    // Notify move completion
                    bitMovedFromTo(*_selectedPiece, *_selectedPieceSource, *targetSquare);
                    
                    // Clear selection
                    _selectedPiece = nullptr;
                    _selectedPieceSource = nullptr;
                    
                    return true;
                }
            }
        }
    }
    
    // If clicking on a non-highlighted square, deselect
    if (_selectedPiece) {
        clearBoardHighlights();
        _selectedPiece = nullptr;
        _selectedPieceSource = nullptr;
    }
    
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) {
        // Highlight valid moves when piece is selected
        std::vector<BitMove>* validMoves = getValidMovesForPiece(bit, src);
        if (validMoves) {
            for (const BitMove& move : *validMoves) {
                ChessSquare* targetSquare = _grid->getSquareByIndex(move.to);
                if (targetSquare) {
                    targetSquare->setHighlighted(true);
                }
            }
        }
        return true;
    }
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // Get valid moves for this piece
    std::vector<BitMove>* validMoves = getValidMovesForPiece(bit, src);
    if (!validMoves || validMoves->empty()) {
        return false;
    }
    
    // Get destination square index
    int toSquare = getSquareIndex(dst);
    if (toSquare < 0 || toSquare >= 64) {
        return false;
    }
    
    // Check if destination is in the list of valid moves
    for (const BitMove& move : *validMoves) {
        if (move.to == toSquare) {
            return true;
        }
    }
    
    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, ChessPiece::Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

std::vector<BitMove>* Chess::generatePossibleMoves(const MoveGenContext& context)
{
    // Initialize knight bitboards once
    if (!_knightBitboardsInitialized) {
        initializeKnightBitboards();
        _knightBitboardsInitialized = true;
    }
    
    _cachedMoves.clear();
    
    // DEBUG: Add this
    std::cout << "generatePossibleMoves: pieceType=";
    switch(context.pieceType) {
        case Pawn: std::cout << "Pawn"; break;
        case Knight: std::cout << "Knight"; break;
        case Bishop: std::cout << "Bishop"; break;
        case King: std::cout << "King"; break;
        default: std::cout << "Unknown(" << (int)context.pieceType << ")"; break;
    }
    std::cout << std::endl;
    
    // Find empty squares
    BitBoard emptySpots = ~(context.blackPieces.getData() | context.whitePieces.getData());
    BitBoard enemyPieces = context.isWhitePlayer ? context.blackPieces : context.whitePieces;
    
    switch (context.pieceType)
    {
        case Knight:
            generateKnightMoves(_cachedMoves, context.pieceBitboard, emptySpots.getData(), enemyPieces.getData());
            break;
        case Pawn:
            std::cout << "Calling generatePawnMoves, moves before: " << _cachedMoves.size() << std::endl;
            generatePawnMoves(_cachedMoves, context.pieceBitboard, emptySpots.getData(), enemyPieces.getData(), context.isWhitePlayer);
            std::cout << "Moves after: " << _cachedMoves.size() << std::endl;
            break;
        case Bishop:
            generateBishopMoves(_cachedMoves, context.pieceBitboard, emptySpots.getData(), context.allPieces.getData(), enemyPieces.getData());
            break;
        case King:
            generateKingMoves(_cachedMoves, context.pieceBitboard, emptySpots.getData(), enemyPieces.getData());
            break;
        // ... other pieces
    }

    return &_cachedMoves;
}

// Initialize precomputed knight move bitboards
void Chess::initializeKnightBitboards() {
    // Knight moves: (±1, ±2) and (±2, ±1)
    int knightOffsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };
    
    for (int square = 0; square < 64; square++) {
        int row = square / 8;
        int col = square % 8;
        uint64_t moves = 0;
        
        for (int i = 0; i < 8; i++) {
            int newRow = row + knightOffsets[i][0];
            int newCol = col + knightOffsets[i][1];
            
            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                int targetSquare = newRow * 8 + newCol;
                moves |= (1ULL << targetSquare);
            }
        }
        
        _knightBitboards[square] = BitBoard(moves);
    }
}

// Generate knight moves
void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitBoard knightBoard, uint64_t emptySquares, uint64_t enemyPieces) {
    knightBoard.forEachBit([&](int fromSquare) {
        // Get all possible knight moves from this square
        uint64_t possibleMoves = _knightBitboards[fromSquare].getData();
        // Filter to only empty squares or enemy pieces (can't capture own pieces)
        uint64_t validTargets = possibleMoves & (emptySquares | enemyPieces);
        
        BitBoard validTargetsBoard(validTargets);
        validTargetsBoard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

// Generate pawn moves
void Chess::generatePawnMoves(std::vector<BitMove>& moves, BitBoard pawnBoard, uint64_t emptySquares, uint64_t enemyPieces, bool isWhite) {
    pawnBoard.forEachBit([&](int fromSquare) {
        int row = fromSquare / 8;
        int col = fromSquare % 8;
        
        if (isWhite) {
            // White pawns move UP (increasing row) towards row 7
            // Forward one square
            if (row < 7) {
                int forwardOne = fromSquare + 8;  // Changed from -8 to +8
                uint64_t forwardOneBit = 1ULL << forwardOne;
                
                if (emptySquares & forwardOneBit) {
                    moves.emplace_back(fromSquare, forwardOne, Pawn);
                    
                    // Forward two squares from starting rank (row 1 = white's starting rank)
                    if (row == 1) {  // Changed from row == 6
                        int forwardTwo = fromSquare + 16;  // Changed from -16 to +16
                        uint64_t forwardTwoBit = 1ULL << forwardTwo;
                        if (emptySquares & forwardTwoBit) {
                            moves.emplace_back(fromSquare, forwardTwo, Pawn);
                        }
                    }
                }
            }
            
            // Capture diagonally (left and right) - moving UP
            if (row < 7) {  // Changed from row > 0
                // Capture left (SW when moving up) - row + 1, col - 1
                if (col > 0) {
                    int captureLeft = fromSquare + 7;  // Changed from -9 to +7
                    uint64_t captureLeftBit = 1ULL << captureLeft;
                    if (enemyPieces & captureLeftBit) {
                        moves.emplace_back(fromSquare, captureLeft, Pawn);
                    }
                }
                // Capture right (SE when moving up) - row + 1, col + 1
                if (col < 7) {
                    int captureRight = fromSquare + 9;  // Changed from -7 to +9
                    uint64_t captureRightBit = 1ULL << captureRight;
                    if (enemyPieces & captureRightBit) {
                        moves.emplace_back(fromSquare, captureRight, Pawn);
                    }
                }
            }
        } else {
            // Black pawns move DOWN (decreasing row) towards row 0
            // Forward one square
            if (row > 0) {  // Changed from row < 7
                int forwardOne = fromSquare - 8;  // Changed from +8 to -8
                uint64_t forwardOneBit = 1ULL << forwardOne;
                
                if (emptySquares & forwardOneBit) {
                    moves.emplace_back(fromSquare, forwardOne, Pawn);
                    
                    // Forward two squares from starting rank (row 6 = black's starting rank)
                    if (row == 6) {  // Changed from row == 1
                        int forwardTwo = fromSquare - 16;  // Changed from +16 to -16
                        uint64_t forwardTwoBit = 1ULL << forwardTwo;
                        if (emptySquares & forwardTwoBit) {
                            moves.emplace_back(fromSquare, forwardTwo, Pawn);
                        }
                    }
                }
            }
            
            // Capture diagonally (left and right) - moving DOWN
            if (row > 0) {  // Changed from row < 7
                // Capture left (NW when moving down) - row - 1, col - 1
                if (col > 0) {
                    int captureLeft = fromSquare - 9;  // Changed from +7 to -9
                    uint64_t captureLeftBit = 1ULL << captureLeft;
                    if (enemyPieces & captureLeftBit) {
                        moves.emplace_back(fromSquare, captureLeft, Pawn);
                    }
                }
                // Capture right (NE when moving down) - row - 1, col + 1
                if (col < 7) {
                    int captureRight = fromSquare - 7;  // Changed from +9 to -7
                    uint64_t captureRightBit = 1ULL << captureRight;
                    if (enemyPieces & captureRightBit) {
                        moves.emplace_back(fromSquare, captureRight, Pawn);
                    }
                }
            }
        }
    });
}

// Generate bishop moves (sliding piece)
void Chess::generateBishopMoves(std::vector<BitMove>& moves, BitBoard bishopBoard, uint64_t emptySquares, uint64_t allPieces, uint64_t enemyPieces) {
    // Diagonal directions: NW, NE, SW, SE
    int directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    
    bishopBoard.forEachBit([&](int fromSquare) {
        int row = fromSquare / 8;
        int col = fromSquare % 8;
        
        // Slide in each of the 4 diagonal directions
        for (int dir = 0; dir < 4; dir++) {
            int r = row + directions[dir][0];
            int c = col + directions[dir][1];
            
            while (r >= 0 && r < 8 && c >= 0 && c < 8) {
                int toSquare = r * 8 + c;
                uint64_t toSquareBit = 1ULL << toSquare;
                
                // Can move to this square (empty or enemy)
                if (emptySquares & toSquareBit) {
                    moves.emplace_back(fromSquare, toSquare, Bishop);
                } else if (enemyPieces & toSquareBit) {
                    // Can capture enemy piece, then stop
                    moves.emplace_back(fromSquare, toSquare, Bishop);
                    break;
                } else {
                    // Hit own piece or blocked, stop
                    break;
                }
                
                // Stop if we hit any piece
                if (allPieces & toSquareBit) {
                    break;
                }
                
                r += directions[dir][0];
                c += directions[dir][1];
            }
        }
    });
}

// Generate king moves
void Chess::generateKingMoves(std::vector<BitMove>& moves, BitBoard kingBoard, uint64_t emptySquares, uint64_t enemyPieces) {
    // King moves one square in any direction (8 directions)
    int kingOffsets[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    kingBoard.forEachBit([&](int fromSquare) {
        int row = fromSquare / 8;
        int col = fromSquare % 8;
        
        for (int i = 0; i < 8; i++) {
            int newRow = row + kingOffsets[i][0];
            int newCol = col + kingOffsets[i][1];
            
            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                int toSquare = newRow * 8 + newCol;
                uint64_t toSquareBit = 1ULL << toSquare;
                
                // Can move to empty square or capture enemy piece
                if ((emptySquares & toSquareBit) || (enemyPieces & toSquareBit)) {
                    moves.emplace_back(fromSquare, toSquare, King);
                }
            }
        }
    });
}

// Helper: Get piece type from bit
ChessPiece Chess::getPieceType(const Bit& bit) const {
    int tag = bit.gameTag();
    if (tag >= 128) {
        tag -= 128;  // Black piece
    }
    
    // DEBUG: Add this temporarily
    std::cout << "getPieceType: gameTag=" << bit.gameTag() << ", extracted=" << tag << ", pieceType=";
    if (tag == 1) std::cout << "Pawn";
    else if (tag == 2) std::cout << "Knight";
    else if (tag == 3) std::cout << "Bishop";
    else if (tag == 4) std::cout << "Rook";
    else if (tag == 5) std::cout << "Queen";
    else if (tag == 6) std::cout << "King";
    else std::cout << "Unknown(" << tag << ")";
    std::cout << std::endl;
    
    return ChessPiece(tag);
}

// Helper: Get square index from holder
int Chess::getSquareIndex(BitHolder& holder) const {
    ChessSquare* square = dynamic_cast<ChessSquare*>(&holder);
    if (square) {
        return square->getSquareIndex();
    }
    return -1;
}

// Helper: Build bitboards for current board state, excluding a specific piece
void Chess::buildBitboards(BitBoard& whitePieces, BitBoard& blackPieces, BitBoard& allPieces, int excludeSquare) {
    whitePieces.setData(0);
    blackPieces.setData(0);
    
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* bit = square->bit();
        if (bit) {
            int squareIndex = square->getSquareIndex();
            
            // Skip the excluded square (the piece we're moving)
            if (squareIndex == excludeSquare) {
                return;
            }
            
            uint64_t squareBit = 1ULL << squareIndex;
            
            if (bit->gameTag() < 128) {
                // White piece
                whitePieces.setData(whitePieces.getData() | squareBit);
            } else {
                // Black piece
                blackPieces.setData(blackPieces.getData() | squareBit);
            }
        }
    });
    
    allPieces.setData(whitePieces.getData() | blackPieces.getData());
}

// Helper: Get valid moves for a specific piece
std::vector<BitMove>* Chess::getValidMovesForPiece(Bit& bit, BitHolder& src) {
    int fromSquare = getSquareIndex(src);
    if (fromSquare < 0 || fromSquare >= 64) {
        return nullptr;
    }
    
    // Build bitboards, excluding the piece we're moving
    BitBoard whitePieces, blackPieces, allPieces;
    buildBitboards(whitePieces, blackPieces, allPieces, fromSquare);
    
    // Determine if this is a white or black piece
    bool isWhite = bit.gameTag() < 128;
    
    // Create bitboard for this specific piece
    uint64_t pieceBit = 1ULL << fromSquare;
    BitBoard pieceBitboard(pieceBit);
    
    // Get piece type
    ChessPiece pieceType = getPieceType(bit);
    
    // Create move generation context
    MoveGenContext context;
    context.pieceBitboard = pieceBitboard;
    context.whitePieces = whitePieces;
    context.blackPieces = blackPieces;
    context.allPieces = allPieces;
    context.pieceType = pieceType;
    context.isWhitePlayer = isWhite;
    
    // Generate moves
    return generatePossibleMoves(context);
}

void Chess::clearBoardHighlights()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->setHighlighted(false);
    });
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // Verify the move was legal (double-check)
    if (!canBitMoveFromTo(bit, src, dst)) {
        // Move was invalid, this shouldn't happen but handle it
        return;
    }
    
    // Clear highlights
    clearBoardHighlights();
    
    // Call parent to end turn
    Game::bitMovedFromTo(bit, src, dst);
}

bool Chess::clickedBit(Bit &bit)
{
    // Get the holder this piece is in
    BitHolder* holder = bit.getHolder();
    if (!holder) {
        return false;
    }
    
    ChessSquare* square = dynamic_cast<ChessSquare*>(holder);
    if (!square) {
        return false;
    }
    
    // If we have a selected piece and clicked on a highlighted square (enemy piece)
    if (_selectedPiece && _selectedPieceSource && square->highlighted()) {
        // Try to move to this square (capture)
        if (canBitMoveFromTo(*_selectedPiece, *_selectedPieceSource, *square)) {
            // Clear highlights
            clearBoardHighlights();
            
            // Handle capture
            if (square->bit()) {
                pieceTaken(square->bit());
            }
            
            // Move the piece
            if (square->dropBitAtPoint(_selectedPiece, square->getPosition())) {
                _selectedPiece->setPosition(square->getPosition());
                
                if (_selectedPieceSource) {
                    _selectedPieceSource->draggedBitTo(_selectedPiece, square);
                }
                
                bitMovedFromTo(*_selectedPiece, *_selectedPieceSource, *square);
                
                _selectedPiece = nullptr;
                _selectedPieceSource = nullptr;
                return true;
            }
        }
    }
    
    // Check if this is the same piece that's already selected
    if (_selectedPiece == &bit) {
        // Deselect
        clearBoardHighlights();
        _selectedPiece = nullptr;
        _selectedPieceSource = nullptr;
        return true;
    }
    
    // Check if we can move this piece (it's the current player's turn)
    if (canBitMoveFrom(bit, *holder)) {
        // Clear previous selection
        if (_selectedPiece) {
            clearBoardHighlights();
        }
        
        // Select this piece
        _selectedPiece = &bit;
        _selectedPieceSource = holder;
        
        // Highlight valid moves (already done in canBitMoveFrom, but ensure it happens)
        std::vector<BitMove>* validMoves = getValidMovesForPiece(bit, *holder);
        if (validMoves) {
            for (const BitMove& move : *validMoves) {
                ChessSquare* targetSquare = _grid->getSquareByIndex(move.to);
                if (targetSquare) {
                    targetSquare->setHighlighted(true);
                }
            }
        }
        
        return true;
    }
    
    // If clicking on an enemy piece or invalid piece, deselect
    if (_selectedPiece) {
        clearBoardHighlights();
        _selectedPiece = nullptr;
        _selectedPieceSource = nullptr;
    }
    
    return false;
}

