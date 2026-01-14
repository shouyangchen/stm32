// //
// // Created by chenshouyang on 2026/1/12.
// //
//
// #include "SeariPortObserver/SerialPortObserver.h"
// #include "SerialPort/SerialPort.h"
//
// Observer::Observer(const uint8_t &priority, Object* parent)
//     : Object(parent, ObjectType::Observer)
//     , priority_m(priority) {
// }
//
// const uint8_t &Observer::getPriority() const {
//     return this->priority_m;
// }
//
// void Observer::event(Event *event) {
//     Object::event(event);
// }
//
// SerialPortObserver::SerialPortObserver()
//     : Object(nullptr, ObjectType::SerialPortObserver) {
// }
//
// void SerialPortObserver::event(Event *event) {
//     Object::event(event);
// }
//
// void SerialPortObserver:: registerSerialPort(SerialPortId portId, SerialPortPtr port) {
//     if (port) {
//         m_serialPorts[portId] = port;
//         port->m_observer = this;
//     }
// }
//
// void SerialPortObserver:: unregisterSerialPort(SerialPortId portId) {
//     auto it = m_serialPorts. find(portId);
//     if (it != m_serialPorts.end()) {
//         if (it->second) {
//             it->second->m_observer = nullptr;
//         }
//         m_serialPorts.erase(it);
//     }
// }
//
// bool SerialPortObserver::getSerialPortState(SerialPortId portId) const {
//     auto it = m_serialPorts.find(portId);
//     if (it != m_serialPorts.end() && it->second) {
//         return it->second->messageReady();
//     }
//     return false;
// }
//
// void SerialPortObserver::deleteObserver(const Observer *observer,
//                                        SerialPortId portId,
//                                        MessageSource source) {
//     if (! observer) return;
//
//     ObserverKey key{portId, source};
//     auto observerQueue = ObserverPriority_queue.find(key);
//
//     if (observerQueue != ObserverPriority_queue. end()) {
//         std::priority_queue<Observer*, std::vector<Observer*, ::allocator<Observer*>>, ObserverCompare> tempQueue;
//
//         while (!observerQueue->second.empty()) {
//             auto it = observerQueue->second.top();
//             if (it != observer) {
//                 tempQueue. push(it);
//             }
//             observerQueue->second.pop();
//         }
//
//         observerQueue->second = std::move(tempQueue);
//     }
// }
//
// void SerialPortObserver:: notifyObservers() {
//     for (auto& [portId, port] : m_serialPorts) {
//         if (port && port->messageReady()) {
//             auto messageSource = port->getMessageSource();
//             if (messageSource != MessageSource::NONE) {
//                 notify(portId, messageSource);
//             }
//         }
//     }
// }
//
// void SerialPortObserver::notifyQueueElements(SerialPortId portId, MessageSource source) {
//     ObserverKey key{portId, source};
//     auto observerQueue = ObserverPriority_queue.find(key);
//
//     if (observerQueue != ObserverPriority_queue.end() && !observerQueue->second. empty()) {
//         auto portIt = m_serialPorts.find(portId);
//         if (portIt == m_serialPorts.end() || ! portIt->second) {
//             return;
//         }
//
//         auto port = portIt->second;
//         const auto& message = port->getMessage(source);
//
//         if (message.source == MessageSource::NONE) {
//             return;
//         }
//
//         const uint8_t* data = message.data_ptr();
//         size_t dataSize = message.size();
//
//         auto tempQueue = observerQueue->second;
//
//         while (!tempQueue.empty()) {
//             auto observerObject = tempQueue.top();
//             if (observerObject) {
//                 observerObject->processSerialData(data, dataSize);
//             }
//             tempQueue.pop();
//         }
//     }
// }
//
// void SerialPortObserver::notify(SerialPortId portId, MessageSource source) {
//     notifyQueueElements(portId, source);
// }
//
// void SerialPortObserver:: installObserver(const Observer *observer,
//                                          SerialPortId portId,
//                                          MessageSource source) {
//     if (!observer) return;
//
//     ObserverKey key{portId, source};
//     auto it = ObserverPriority_queue.find(key);
//
//     if (it != ObserverPriority_queue.end()) {
//         it->second.push(const_cast<Observer *>(observer));
//     } else {
//         std::priority_queue<Observer*, std::vector<Observer*, ::allocator<Observer*>>, ObserverCompare> newQueue;
//         newQueue.push(const_cast<Observer *>(observer));
//         ObserverPriority_queue. insert_or_assign(key, std::move(newQueue));
//     }
// }