//
// Created by chenshouyang on 2026/1/11.
//

#ifndef SPLPROJECT_ACTION_H
#define SPLPROJECT_ACTION_H
#include <vector>

#include "Servo/Servo.h"
#include "Allocator/allocator.h"
#include "singleton/Singleton.h"
#include "Object.h"
class Action :public Object,public Singleton<Action>{
public:
    enum class StateGoForward:uint8_t{
        INIT=0,
        PHASE_1,
        PHASE_2,
        PHASE_3,
        COMPLETE
    };

    enum class StateGoBack:uint8_t{
        INIT=0,
        PHASE_1,
        PHASE_2,
        PHASE_3,
        COMPLETE
    };

    enum class StateGoRight:uint8_t{
        INIT=0,
        PHASE_1,
        PHASE_2,
        PHASE_3,
        COMPLETE
    };

    enum class StateGoLeft:uint8_t{
        INIT=0,
        PHASE_1,
        PHASE_2,
        PHASE_3,
        COMPLETE
    };

    Action(const Action&)=delete;
    Action(Action&&)=delete;
    Action& operator=(const Action&)=delete;
    Action(Action&)=delete;
    void event(Event *event) override;
    void goForward();
    void goBack();
    void goLeft();
    void goRight();
    [[nodiscard]] StateGoForward getGOForWardState()const;
    [[nodiscard]] StateGoRight getGoRightState()const;
    [[nodiscard]] StateGoBack getGoBackState()const;
    [[nodiscard]] StateGoLeft getGoLeftState()const;
private:
    friend class Singleton<Action>;
    friend void GoForward(void*data);
    Action();
    void go_Forward();//向前
    void go_Left();//向左
    void go_Back();//退后
    void go_Right();//向右
    std::vector<Servo,::allocator<Servo>>legs_m;
    StateGoBack go_back_state;
    StateGoForward go_forward_state;
    StateGoLeft go_left_state;
    StateGoRight go_right_right;
    uint16_t time_space;
};


#endif //SPLPROJECT_ACTION_H