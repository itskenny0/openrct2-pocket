// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "PathSelection.h"
#include <optional>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/world/Footpath.h>

namespace OpenRCT2::Ui::Handheld::Paths
{
    struct BatchMaterial
    {
        ObjectEntryIndex surface{}, railings{};
        PathConstructFlags flags{};
        bool operator==(const BatchMaterial&) const = default;
    };
    class BatchDraft
    {
    public:
        BuildMode mode{ BuildMode::single };
        std::optional<CoordsXYZ> start;
        std::optional<CoordsXY> end;
        BatchMaterial material;
        Selection selection;
        std::vector<bool> valid;
        GameActions::Result quote;
        bool review{}, busy{};
        size_t completed{};
        money64 spent{};
        std::string notice;
        uint64_t validationCount{};

        void reset()
        {
            if (busy) return;
            *this = {};
        }
        void invalidate() { cached = false; review = false; }
        bool cancel()
        {
            if (busy) return true;
            if (review) { review = false; return true; }
            if (start) { start.reset(); end.reset(); invalidate(); notice.clear(); return true; }
            return false;
        }
        void update(std::optional<CoordsXY> destination, BatchMaterial settings, bool force = false)
        {
            auto& state = getGameState();
            if (busy || !start) return;
            if (!force && cached && end == destination && material == settings && cash == state.park.cash
                && paused == gGamePaused && state.currentTicks - tick < 8) return;
            if (end != destination || material != settings) review = false;
            end = destination; material = settings; cash = state.park.cash; paused = gGamePaused; tick = state.currentTicks;
            cached = true; ++validationCount;
            selection = {}; valid.clear(); quote = {};
            if (!end) { fail("Move destination onto the park"); return; }
            selection = SelectTiles(mode, *start, *end);
            if (!selection.error.empty()) { fail(selection.error); return; }
            GameActions::Result firstError;
            for (const auto& tile : selection.tiles)
            {
                auto action = makeAction(tile);
                const auto result = GameActions::Query(&action, state);
                valid.push_back(result.error == GameActions::Status::ok);
                if (result.error == GameActions::Status::ok) quote.cost += result.cost;
                else if (firstError.error == GameActions::Status::ok) firstError = result;
            }
            quote.expenditure = ExpenditureType::landscaping;
            if (firstError.error != GameActions::Status::ok)
            {
                const auto cost = quote.cost;
                quote = firstError; quote.cost = cost; // Partial quote never advertised as a total.
            }
            else if (!FinanceCheckAffordability(quote.cost, {}))
            {
                quote.error = GameActions::Status::insufficientFunds;
                quote.errorMessage = "Not enough cash for the whole selection";
            }
        }
        GameActions::Result confirm()
        {
            if (busy) return error("Building selection; please wait");
            update(end, material, true); // Fresh quote, not an atomicity guarantee.
            if (review && reviewedCost != quote.cost) review = false;
            if (quote.error != GameActions::Status::ok) { review = false; return quote; }
            if (!review) { review = true; reviewedCost = quote.cost; return quote; }
            review = false; busy = true; completed = 0; spent = 0; notice.clear();
            pending = selection.tiles;
            executingState = &getGameState();
            placeNext();
            return quote;
        }
    private:
        bool cached{};
        uint32_t tick{};
        money64 cash{}, reviewedCost{};
        uint8_t paused{};
        std::vector<CoordsXYZ> pending;
        const GameState_t* executingState{};
        static GameActions::Result error(const std::string& message)
        {
            GameActions::Result r; r.error = GameActions::Status::invalidParameters; r.errorMessage = message; return r;
        }
        void fail(const std::string& message) { quote = error(message); }
        GameActions::FootpathPlaceAction makeAction(CoordsXYZ tile) const
        {
            return GameActions::FootpathPlaceAction(tile, { FootpathSlopeType::flat, 0 }, material.surface,
                material.railings, kInvalidDirection, material.flags);
        }
        void stop(const GameActions::Result& result)
        {
            busy = false;
            notice = "Stopped: " + std::to_string(completed) + "/" + std::to_string(pending.size())
                + " built. " + result.getErrorMessage();
            // Never auto-retry completed purchases. A new anchor is required.
            start.reset(); end.reset(); invalidate();
        }
        void placeNext()
        {
            if (executingState != &getGameState()) { stop(error("Park changed; remaining tiles cancelled")); return; }
            auto action = makeAction(pending[completed]);
            action.SetCallback([this](const auto*, const GameActions::Result* result) {
                if (result->error != GameActions::Status::ok) { stop(*result); return; }
                spent += result->cost; ++completed;
                if (completed == pending.size())
                {
                    busy = false; notice = "Built " + std::to_string(completed) + " tiles";
                    start.reset(); end.reset(); invalidate();
                }
                else placeNext();
            });
            const auto result = GameActions::Execute(&action, getGameState());
            if (busy && result.error != GameActions::Status::ok) stop(result);
        }
    };
}
