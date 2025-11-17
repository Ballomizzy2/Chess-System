#pragma once

#include "Game.h"
#include "Grid.h"
#include "BitBoard.h"


constexpr int pieceSize = 80;

struct MoveGenContext {
        BitBoard pieceBitboard;        // The specific piece(s) to generate moves for
        BitBoard whitePieces;
        BitBoard blackPieces;
        BitBoard allPieces;            // whitePieces | blackPieces (helpful for blocking)
        ChessPiece pieceType;
        bool isWhitePlayer;            // To determine which pieces are enemies
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    bool clickedBit(Bit &bit) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;
    
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }
    
    // generating moves
    std::vector<BitMove>* generatePossibleMoves(const MoveGenContext& context);
    void generateKnightMoves(std::vector<BitMove>& moves, BitBoard knightBoard, uint64_t emptySquares, uint64_t enemyPieces);
    void generatePawnMoves(std::vector<BitMove>& moves, BitBoard pawnBoard, uint64_t emptySquares, uint64_t enemyPieces, bool isWhite);
    void generateBishopMoves(std::vector<BitMove>& moves, BitBoard bishopBoard, uint64_t emptySquares, uint64_t allPieces, uint64_t enemyPieces);
    void generateKingMoves(std::vector<BitMove>& moves, BitBoard kingBoard, uint64_t emptySquares, uint64_t enemyPieces);

    

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

     // Precomputed knight move bitboards
     static BitBoard _knightBitboards[64];
     static bool _knightBitboardsInitialized;
     static void initializeKnightBitboards();

    // Helper methods for move generation
    void buildBitboards(BitBoard& whitePieces, BitBoard& blackPieces, BitBoard& allPieces, int excludeSquare = -1);
    ChessPiece getPieceType(const Bit& bit) const;
    int getSquareIndex(BitHolder& holder) const;
    std::vector<BitMove>* getValidMovesForPiece(Bit& bit, BitHolder& src);
    void clearBoardHighlights() override;

    // Cache for move generation
    std::vector<BitMove> _cachedMoves;
    
    // Click-and-drop selection tracking
    Bit* _selectedPiece;
    BitHolder* _selectedPieceSource;
    
    Grid* _grid;
};