//
// Created by chenshouyang on 2025/12/19.
//
#include "EventQueue/Event.h"

Event::Event(EventType type, Object *object):type_m(type),reciver_m(object){

}


Event::EventType Event::getEventType() const{
    return this->type_m;
}

Object* Event::getReciver() const{
    return this->reciver_m;
}