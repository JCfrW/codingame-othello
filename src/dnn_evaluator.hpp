#ifndef _DNN_EVALUATOR_
#define _DNN_EVALUATOR_

#include "board.hpp"

class DNNEvaluatorResult
{
public:
    float policy_logits[BOARD_AREA];
    float value_logit;
};

class DNNEvaluatorRequest
{
public:
    float board_repr[BOARD_AREA * 3];
};

class DNNEvaluator
{
public:
    virtual DNNEvaluatorResult evaluate(const Board &board) = 0;

protected:
    DNNEvaluatorRequest make_request(const Board &board)
    {
        DNNEvaluatorRequest req;
        memset(&req, 0, sizeof(req));
        for (int i = 0; i < N_PLAYER; i++)
        {
            int turn = i == 0 ? board.turn() : 1 - board.turn();
            BoardPlane bb = board.plane(turn);
            for (int pos = 0; pos < BOARD_AREA; pos++)
            {
                if (bb & (1 << pos))
                {
                    req.board_repr[i * BOARD_AREA + pos] = 1.0F;
                }
            }
        }
        // fill 1
        for (int pos = 0; pos < BOARD_AREA; pos++)
        {
            req.board_repr[2 * BOARD_AREA + pos] = 1.0F;
        }

        return req;
    }
};
#endif
