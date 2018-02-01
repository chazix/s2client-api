#include "test_sendmapcommand.h"
#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "bot_examples.h"

#include <iostream>

namespace sc2 {
    static const unsigned CommandStepCount = 500;

    struct CommandInfo {
        CommandInfo(SC2APIProtocol::RequestMapCommand::CommandChoiceCase choice, const std::string& cmdid = std::string()) :
            m_choice(choice), m_cmdid(cmdid) {
        }

        SC2APIProtocol::RequestMapCommand::CommandChoiceCase m_choice;
        std::string m_cmdid;
    };

    class SendCommandBot : public MarineMicroBot {
    public:
        SendCommandBot(Coordinator& coordinator) : MarineMicroBot(&coordinator), m_coordinator(coordinator) {
            m_commands = {
                CommandInfo(SC2APIProtocol::RequestMapCommand::kRestartGame, "Reset"),
                CommandInfo(SC2APIProtocol::RequestMapCommand::kCustom, "displace")
            };
        }

        virtual void OnStep() {
            if (++m_numSteps == CommandStepCount) {
                const CommandInfo& cmdinfo = m_commands[m_commandIndex];
                //std::cout << "Sending MapCommand: (choice: " << cmdinfo.m_choice << "), (cmdid: " << cmdinfo.m_cmdid << ")" << std::endl;
                //m_coordinator.SendMapCommand(cmdinfo.m_choice, cmdinfo.m_cmdid);
                m_commandIndex = (m_commandIndex + 1) % m_commands.size();
                m_numSteps = 0;
            }

            MarineMicroBot::OnStep();
        }

    private:
        unsigned m_numSteps{0};
        unsigned m_commandIndex{0};
        Coordinator& m_coordinator;
        std::vector<CommandInfo> m_commands;
    };

    void CoordinatorOnGameEnd(Coordinator* coordinator) {
        std::cout << "Executing Coordinator OnGameEnd Functionality" << std::endl;
        std::cout << "Sending MapCommand: (choice: " << "---" << "), (cmdid: Reset)" << std::endl;
        coordinator->SendMapCommand(SC2APIProtocol::RequestMapCommand::kRestartGame);
    }

    bool TestSendMapCommand(int argc, char** argv) {
        Coordinator coordinator;
        if (!coordinator.LoadSettings(argc, argv)) {
            return false;
        }

        coordinator.RegisterOnGameEndCallback(CoordinatorOnGameEnd);
        coordinator.SetRealtime(true);
        coordinator.SetMultithreaded(true);
        SendCommandBot mapCommandBot(coordinator);
        Agent nothingBot(&coordinator);
        coordinator.SetParticipants({
            CreateParticipant(sc2::Race::Terran, &mapCommandBot),
            CreateParticipant(sc2::Race::Zerg, &nothingBot),
        });

        coordinator.LaunchStarcraft();
        coordinator.StartGame(sc2::kMapFastRestartMultiplayer);

        while (true) {
            coordinator.Update();
            continue;
        }

        return true;
    }
}