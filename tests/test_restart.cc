#include "test_movement_combat.h"
#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "bot_examples.h"
#include <iostream>
#include <string>
#include <random>

namespace sc2 {

static const int NumRestartsToTest = 2;


//
// DoSomethingBot
//

class DoSomethingBot : public Agent {
public:
    int count_restarts_;
    GameInfo game_info_;
    uint32_t trigger_on_gameloop_;
    uint32_t finish_by_gameloop_;
    Point2D battle_pt_;
    bool success_;

    DoSomethingBot() :
        count_restarts_(0),
        success_(true) {
    }

    bool IsFinished() const {
        return count_restarts_ >= NumRestartsToTest;
    }

    void OnGameFullStart() final {
        game_info_ = Observation()->GetGameInfo();
        Debug()->DebugShowMap();
        Debug()->DebugEnemyControl();
        Debug()->SendDebug();
    }

    void OnGameStart() final {
        trigger_on_gameloop_ = Observation()->GetGameLoop() + 10;
        finish_by_gameloop_ = Observation()->GetGameLoop() + 2000;


        Point2D friendly_rally_pt = FindRandomLocation(game_info_);
        Point2D enemy_rally_pt = FindRandomLocation(game_info_);
        battle_pt_.x = (friendly_rally_pt.x + enemy_rally_pt.x) / 2.0f;
        battle_pt_.y = (friendly_rally_pt.y + enemy_rally_pt.y) / 2.0f;

        Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_MARINE, friendly_rally_pt, Observation()->GetPlayerID(), 20);
        Debug()->DebugCreateUnit(UNIT_TYPEID::TERRAN_MARINE, enemy_rally_pt, Observation()->GetPlayerID() + 1, 20);
        Debug()->SendDebug();
    }

    void OnStep() final {
        const ObservationInterface* obs = Observation();
        if (obs->GetGameLoop() >= finish_by_gameloop_) {
            Debug()->DebugEndGame(true);
            Debug()->SendDebug();
            return;
        }

        if (obs->GetGameLoop() != trigger_on_gameloop_)
            return;

        ActionInterface* act = Actions();
        const Units& units = obs->GetUnits();
        for (const auto& unit : units) {
            act->UnitCommand(unit, ABILITY_ID::SMART, battle_pt_);
        }
    }

    void OnGameEnd() final {
        ++count_restarts_;
        std::cout << "Restart test: " << std::to_string(count_restarts_) << " of " <<
            std::to_string(NumRestartsToTest) << " complete." << std::endl;
        if (!IsFinished()) {
            AgentControl()->Restart();
            AgentControl()->WaitForRestart();
        }
    }
};

class MarineMicroBotReset : public MarineMicroBot {
public:
    MarineMicroBotReset() : MarineMicroBot() {}

    virtual void OnGameEnd() override;
    int& GetNumMultiplayerRestarts() { return multiPlayerRestarts; }

private:
    int multiPlayerRestarts{0};
};

void MarineMicroBotReset::OnGameEnd() {
    ++multiPlayerRestarts;
    std::cout << "Restart test: " << std::to_string(multiPlayerRestarts) << " of " <<
        std::to_string(NumRestartsToTest) << " complete." << std::endl;
}

//
// TestMovementCombat
//

bool TestRequestRestartGame(int argc, char** argv) {
    Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv)) {
        return false;
    }

    // Add the custom bot, it will control the players.
    DoSomethingBot bot;

    /* ----------------------------------------------------------- */
    // Single-player Non-Real-Time Restart Tests
    /* ----------------------------------------------------------- */
    coordinator.SetParticipants({
        CreateParticipant(sc2::Race::Terran, &bot),
    });

    /* Test non multi-threaded non real-time single player restart */
    std::cout << "    Testing non multi-threaded non real-time singleplayer restart" << std::endl;
    // Start the game.
    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapEmpty);

    // Step forward the game simulation.
    while (!bot.IsFinished()) {
        coordinator.Update();
    }
    /* ----------------------------------------------------------- */

    bot.AgentControl()->Restart();
    bot.AgentControl()->WaitForRestart();

    /* Test multi-threaded non real-time single player restart */
    std::cout << "    Testing multi-threaded non real-time singleplayer restart" << std::endl;
    bot.count_restarts_ = 0;
    coordinator.SetMultithreaded(true);

    // Step forward the game simulation.
    while (!bot.IsFinished()) {
        coordinator.Update();
    }
    /* ----------------------------------------------------------- */

    coordinator.TerminateStarcraft();

    /* ----------------------------------------------------------- */
    // Single-player Real-Time Restart Tests
    /* ----------------------------------------------------------- */
    coordinator.SetRealtime(true);
    coordinator.SetParticipants({
        CreateParticipant(sc2::Race::Terran, &bot),
    });

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapEmpty);

    /* Test non multi-threaded real-time single player restart */
    std::cout << "    Testing non multi-threaded real-time singleplayer restart" << std::endl;
    bot.count_restarts_ = 0;
    coordinator.SetMultithreaded(false);

    // Step forward the game simulation.
    while (!bot.IsFinished()) {
        coordinator.Update();
    }
    /* ----------------------------------------------------------- */

    bot.AgentControl()->Restart();
    bot.AgentControl()->WaitForRestart();

    /* Test multi-threaded real-time single player restart */
    std::cout << "    Testing multi-threaded real-time singleplayer restart" << std::endl;
    bot.count_restarts_ = 0;
    coordinator.SetMultithreaded(true);

    // Step forward the game simulation.
    while (!bot.IsFinished()) {
        coordinator.Update();
    }
    /* ----------------------------------------------------------- */

    coordinator.TerminateStarcraft();

    /* ----------------------------------------------------------- */
    // Multi-player Non-Real-Time Restart Tests
    /* ----------------------------------------------------------- */
    sc2::MarineMicroBotReset microBot;
    sc2::Agent nothingBot;

    coordinator.SetRealtime(false);
    coordinator.SetParticipants({
        CreateParticipant(sc2::Race::Terran, &microBot),
        CreateParticipant(sc2::Race::Zerg, &nothingBot)
    });
    coordinator.RegisterForRestartGame(true);

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapFastRestartMultiplayer);

    /* Test non multi-threaded non real-time multiplayer restart */
    std::cout << "    Test non multi-threaded non real-time multiplayer restart" << std::endl;
    coordinator.SetMultithreaded(false);

    // Step forward the game simulation.
    while (coordinator.Update()) {
        if (microBot.GetNumMultiplayerRestarts() >= NumRestartsToTest) {
            break;
        }
    }

    coordinator.LeaveGame();
    coordinator.WaitForAllResponses();
    coordinator.CreateGame(sc2::kMapFastRestartMultiplayer);
    coordinator.JoinGame();
    /* ----------------------------------------------------------- */

    /* Test multi-threaded non real-time multiplayer restart */
    std::cout << "    Test multi-threaded non real-time multiplayer restart" << std::endl;
    microBot.GetNumMultiplayerRestarts() = 0;
    coordinator.SetMultithreaded(true);

    // Step forward the game simulation.
    while (coordinator.Update()) {
        if (microBot.GetNumMultiplayerRestarts() >= NumRestartsToTest) {
            break;
        }
    }
    /* ----------------------------------------------------------- */

    coordinator.TerminateStarcraft();

    /* ----------------------------------------------------------- */
    // Multi-player Real-Time Restart Tests
    /* ----------------------------------------------------------- */
    coordinator.SetRealtime(true);
    coordinator.SetParticipants({
        CreateParticipant(sc2::Race::Terran, &microBot),
        CreateParticipant(sc2::Race::Zerg, &nothingBot)
    });
    coordinator.RegisterForRestartGame(true);

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapFastRestartMultiplayer);

    /* Test non multi-threaded real-time multiplayer restart */
    std::cout << "    Test non multi-threaded real-time multiplayer restart" << std::endl;
    microBot.GetNumMultiplayerRestarts() = 0;
    coordinator.SetMultithreaded(false);

    // Step forward the game simulation.
    while (coordinator.Update()) {
        if (microBot.GetNumMultiplayerRestarts() >= NumRestartsToTest) {
            break;
        }
    }

#if 0
    coordinator.LeaveGame();
    coordinator.WaitForAllResponses();
    coordinator.CreateGame(sc2::kMapFastRestartMultiplayer);
    coordinator.JoinGame();
    /* ----------------------------------------------------------- */

    /* Test multi-threaded real-time multiplayer restart */
    std::cout << "    Test multi-threaded real-time multiplayer restart" << std::endl;
    microBot.GetNumMultiplayerRestarts() = 0;
    coordinator.SetMultithreaded(true);

    // Step forward the game simulation.
    while (coordinator.Update()) {
        if (microBot.GetNumMultiplayerRestarts() >= NumRestartsToTest) {
            break;
        }
    }
    /* ----------------------------------------------------------- */
#endif

    return bot.success_;
}

}

