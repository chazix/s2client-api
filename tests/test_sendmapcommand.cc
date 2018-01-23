#include "test_sendmapcommand.h"

#include <iostream>

namespace sc2 {
    static const unsigned CommandStepCount = 1000;

    class SendCommandBot : public Agent {
    public:
        SendCommandBot(Coordinator& coordinator) : Agent(), m_coordinator(coordinator) {
            m_commands = {
                "reset",
                "invalidcmd"
            };
        }

        virtual void OnStep() {
            if (++m_numSteps == CommandStepCount) {
                std::cout << "Sending MapCommand: " << m_commands[m_commandIndex] << std::endl;
                m_coordinator.SendMapCommand(m_commands[m_commandIndex]);
                m_commandIndex = (m_commandIndex + 1) % m_commands.size();
                m_numSteps = 0;
            }
        }

    private:
        unsigned m_numSteps{0};
        unsigned m_commandIndex{0};
        Coordinator& m_coordinator;
        std::vector<std::string> m_commands;
    };

    bool TestSendMapCommand(int argc, char** argv) {
        Coordinator coordinator;
        if (!coordinator.LoadSettings(argc, argv)) {
            return false;
        }

        SendCommandBot mapCommandBot(coordinator);
        coordinator.SetParticipants({
            CreateParticipant(sc2::Race::Terran, &mapCommandBot),
        });

        coordinator.LaunchStarcraft();
        coordinator.StartGame(sc2::kMapFastRestartMultiplayer);

        while (coordinator.Update()) {
            continue;
        }

        return true;
    }
}