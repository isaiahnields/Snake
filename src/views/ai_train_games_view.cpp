#include "ai_train_games_view.h"

#include "../view_manager.h"

AITrainGamesView::AITrainGamesView(SessionState &session)
    : MenuView(MenuPage::Train), session_(session)
{
}

bool AITrainGamesView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::Train;
}

std::string AITrainGamesView::title() const
{
    return "TRAIN ACTIVE AI";
}

std::vector<std::string> AITrainGamesView::details() const
{
    return {"AI training is not implemented.",
            "Episode count and render mode are session preferences only."};
}

std::vector<MenuItem> AITrainGamesView::items() const
{
    return {menu_command("Episodes to Run: " + std::to_string(session_.training_episodes), page,
                         MenuAction::CycleTrainingEpisodes),
            menu_command(session_.visual_training
                             ? "Render Mode: Visual (Slow to watch)"
                             : "Render Mode: Headless (Fast, no game rendering)",
                         page, MenuAction::ToggleTrainingRenderMode),
            menu_unavailable("Start Training Session", page),
            menu_link("Return to AI Control Center", MenuPage::AI)};
}

void AITrainGamesView::perform(MenuAction action, ViewManager &manager)
{
    (void)manager;
    switch (action)
    {
    case MenuAction::CycleTrainingEpisodes:
        session_.training_episodes = session_.training_episodes == 1000  ? 100
                                     : session_.training_episodes == 100 ? 10
                                                                         : 1000;
        notice.clear();
        break;
    case MenuAction::ToggleTrainingRenderMode:
        session_.visual_training = !session_.visual_training;
        notice.clear();
        break;
    default:
        break;
    }
}
