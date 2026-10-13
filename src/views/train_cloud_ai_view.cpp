#include "train_cloud_ai_view.h"

TrainCloudAIView::TrainCloudAIView() : MenuView(MenuPage::CloudTraining) {}

bool TrainCloudAIView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::CloudTraining;
}

std::string TrainCloudAIView::title() const
{
    return "TRAIN ACTIVE AI IN CLOUD";
}

std::vector<std::string> TrainCloudAIView::details() const
{
    return {"Cloud training runs episodes on a remote training service.",
            "No cloud service is configured; cloud training is not implemented.",
            "Use Train Active AI to train locally on this device."};
}

std::vector<MenuItem> TrainCloudAIView::items() const
{
    return {menu_unavailable("Cloud Provider: <none configured>", page),
            menu_unavailable("Start Cloud Training Session", page),
            menu_link("Return to AI Control Center", MenuPage::AI)};
}
