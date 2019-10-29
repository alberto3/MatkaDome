#ifndef MATKA_H
#define MATKA_H

#include <Arduino.h>
#include <NodeID.h>

class Matka {
public:
    Matka() = default;

    virtual ~Matka() {}

    void SetID(NodeID *i_ID) {
        m_NodeID = i_ID;
    }

    uint8_t GetNodeId() const {
        return m_NodeID->getID();
    }

    uint8_t GetServerID() const {
        return m_ServerID;
    }

    void SetServerID(uint8_t i_ServerID) {
        m_ServerID = i_ServerID;
    }

    uint8_t GetNumOfPlayersInc() const {
        return m_NodeID->getN();
    }

    uint16_t GetTargets() const {
        return m_Targets;
    }

    void SetTargets(uint16_t i_Targets) {
        m_Targets = i_Targets;
    }

    void AddTargets(uint16_t i_Targets) {
        m_Targets |= i_Targets;
    }

    void SetTargetID(uint8_t i_TargetID) {
        m_Targets |= 1u << (i_TargetID - 1u);
    }

    void ClearTargetID(uint8_t i_TargetID) {
        m_Targets &= ~(1u << (i_TargetID - 1u));
    }

    void ClearTargets() {
        m_Targets = 0;
    }

    void SetRandomTargets(uint8_t i_HowMany, boolean i_Multiple = false) {
        uint8_t numOfPlayers = GetNumOfPlayersInc();
        if (numOfPlayers > 1 && m_Targets < (1u << numOfPlayers)) {
            uint32_t time = millis();
            while (i_HowMany > 0 && millis() - time < 1000) {
                uint8_t targetID;

                do {
                    targetID = random(0, numOfPlayers);
                } while (m_NodeID->getID() == targetID);

                uint8_t targetIndex = 1u << targetID;

                if (true/*i_Multiple || (targetIndex & m_Targets) == 0*/) {
//                    m_Targets |= targetIndex;
                    m_Targets = targetIndex;
                    i_HowMany--;
                }
            }
        }
    }

protected:
    NodeID *m_NodeID{};
    uint8_t m_ServerID{};
    uint16_t m_Targets{};

};


#endif
