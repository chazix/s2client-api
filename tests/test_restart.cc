#include "test_movement_combat.h"
#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
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

#if 0
class MarineMicroBot : public Agent {
public:
    virtual void OnGameStart();
    virtual void OnStep() final;
    virtual void OnUnitDestroyed(const Unit* unit) override;
    virtual void OnGameEnd() override;

    int& GetNumMultiplayerRestarts() { return multiPlayerRestarts; }

private:
    bool GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D& position);
    bool GetNearestZergling(const Point2D& from);

    const Unit* targeted_zergling_;
    bool move_back_;
    Point2D backup_target_;
    Point2D backup_start_;
    int multiPlayerRestarts{0};
};

void MarineMicroBot::OnGameStart() {
    move_back_ = false;
    targeted_zergling_ = 0;
}

void MarineMicroBot::OnStep() {
    const ObservationInterface* observation = Observation();
    ActionInterface* action = Actions();

    Point2D mp, zp;

    if (!GetPosition(UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, mp)) {
        return;
    }

    if (!GetPosition(UNIT_TYPEID::ZERG_ZERGLING, Unit::Alliance::Enemy, zp)) {
        return;
    }

    if (!GetNearestZergling(mp)) {
        return;
    }

    Units units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto& u : units) {
        switch (static_cast<UNIT_TYPEID>(u->unit_type)) {
        case UNIT_TYPEID::TERRAN_MARINE: {
            if (!move_back_) {
                action->UnitCommand(u, ABILITY_ID::ATTACK, targeted_zergling_);
            }
            else {
                if (Distance2D(mp, backup_target_) < 1.5f) {
                    move_back_ = false;
                }

                action->UnitCommand(u, ABILITY_ID::SMART, backup_target_);
            }
            break;
        }
        default: {
            break;
        }
        }
    }
}

void MarineMicroBot::OnUnitDestroyed(const Unit* unit) {
    if (unit == targeted_zergling_) {
        Point2D mp, zp;
        if (!GetPosition(UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, mp)) {
            return;
        }

        if (!GetPosition(UNIT_TYPEID::ZERG_ZERGLING, Unit::Alliance::Enemy, zp)) {
            return;
        }

        Vector2D diff = mp - zp;
        Normalize2D(diff);

        targeted_zergling_ = 0;
        move_back_ = true;
        backup_start_ = mp;
        backup_target_ = mp + diff * 3.0f;
    }
}

void MarineMicroBot::OnGameEnd() {
    ++multiPlayerRestarts;
    std::cout << "Restart test: " << std::to_string(multiPlayerRestarts) << " of " <<
        std::to_string(NumRestartsToTest) << " complete." << std::endl;
}

bool MarineMicroBot::GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D& position) {
    const ObservationInterface* observation = Observation();
    Units units = observation->GetUnits(alliace);

    if (units.empty()) {
        return false;
    }

    position = Point2D(0.0f, 0.0f);
    unsigned int count = 0;

    for (const auto& u : units) {
        if (u->unit_type == unit_type) {
            position += u->pos;
            ++count;
        }
    }

    position /= (float)count;

    return true;
}

bool MarineMicroBot::GetNearestZergling(const Point2D& from) {
    const ObservationInterface* observation = Observation();
    Units units = observation->GetUnits(Unit::Alliance::Enemy);

    if (units.empty()) {
        return false;
    }

    float distance = std::numeric_limits<float>::max();
    for (const auto& u : units) {
        if (u->unit_type == UNIT_TYPEID::ZERG_ZERGLING) {
            float d = DistanceSquared2D(u->pos, from);
            if (d < distance) {
                distance = d;
                targeted_zergling_ = u;
            }
        }
    }

    return true;
}
#endif

//
// TestMovementCombat
//

bool TestRequestRestartGame(int argc, char** argv) {
#if 0
    Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv)) {
        return false;
    }

    // Add the custom bot, it will control the players.
    DoSomethingBot bot;

#if 0
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
    coordinator.StartGame(sc2::kMapFastRestartMultiplayer);

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
    coordinator.StartGame(sc2::kMapFastRestartMultiplayer);

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
#endif
    /* ----------------------------------------------------------- */
    // Multi-player Non-Real-Time Restart Tests
    /* ----------------------------------------------------------- */
    sc2::MarineMicroBot microBot;
    sc2::Agent nothingBot;

    coordinator.SetRealtime(false);
    coordinator.SetParticipants({
        CreateParticipant(sc2::Race::Terran, &microBot),
        CreateParticipant(sc2::Race::Zerg, &nothingBot)
    });
    coordinator.RegisterForRestartGame(true);

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapFastRestartMultiplayer);

#if 0
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
#endif
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
#endif
    return true;
}

}

