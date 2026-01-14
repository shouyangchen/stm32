// //
// // Created by chenshouyang on 2026/1/12.
// //
//
// #ifndef SPLPROJECT_SEARIPORTOBSERVER_H
// #define SPLPROJECT_SEARIPORTOBSERVER_H
//
// #include <memory>
// #include <queue>
// #include "Object.h"
// #include <unordered_map>
// #include <utility>
// #include "SerialPort/SerialPort.h"
// #include "singleton/Singleton.h"
//
// template<typename AllocatorType>
// class SerialPort;
//
// // 观察者基类
// class Observer :  public Object {
// public:
//     static constexpr ObjectType staticObjectType() {
//         return ObjectType::Observer;
//     }
//
//     explicit Observer(const uint8_t& priority, Object* parent = nullptr);
//     void event(Event *event) override;
//     virtual void processSerialData(const uint8_t*, size_t size) = 0;
//     [[nodiscard]] const uint8_t& getPriority() const;
//     ~Observer() override = default;
//
// private:
//     uint8_t priority_m;
// };
//
// struct ObserverCompare {
//     bool operator()(Observer* lhs, Observer* rhs) const {
//         if (! lhs || !rhs) return false;
//         return lhs->getPriority() < rhs->getPriority();
//     }
// };
//
// // 串口观察者
// class SerialPortObserver : public Object, public Singleton<SerialPortObserver> {
// public:
//     static constexpr ObjectType staticObjectType() {
//         return ObjectType::SerialPortObserver;
//     }
//
//     using MessageSource = SerialPort<>:: MessageSource;
//     using SerialPortPtr = SerialPort<>*;
//
//     void event(Event *event) override;
//     void notifyObservers();
//
//     void installObserver(const Observer* observer,
//                         SerialPortId portId,
//                         MessageSource source);
//
//     void deleteObserver(const Observer* observer,
//                        SerialPortId portId,
//                        MessageSource source);
//
//     void registerSerialPort(SerialPortId portId, SerialPortPtr port);
//     void unregisterSerialPort(SerialPortId portId);
//
//     [[nodiscard]] bool getSerialPortState(SerialPortId portId) const;
//
//     ~SerialPortObserver() override = default;
//
// private:
//     friend class Singleton<SerialPortObserver>;
//     template<typename AllocatorType>
//     friend class SerialPort;
//
//     SerialPortObserver();
//
//     void notifyQueueElements(SerialPortId portId, MessageSource source);
//     void notify(SerialPortId portId, MessageSource source);
//
//     using ObserverKey = std::pair<SerialPortId, MessageSource>;
//
//     struct ObserverKeyHash {
//         std::size_t operator()(const ObserverKey& key) const {
//             return std::hash<uint8_t>()(static_cast<uint8_t>(key.first)) ^
//                    (std::hash<uint8_t>()(static_cast<uint8_t>(key. second)) << 1);
//         }
//     };
//
//     std::unordered_map<SerialPortId, SerialPortPtr> m_serialPorts;
//
//     std::unordered_map<
//         ObserverKey,
//         std::priority_queue<Observer*, std::vector<Observer*, :: allocator<Observer*>>, ObserverCompare>,
//         ObserverKeyHash
//     > ObserverPriority_queue;
// };
//
// #endif //SPLPROJECT_SEARIPORTOBSERVER_H