// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Finances.h"
#include "FinanceInformation.h"
#include "MarketingManagement.h"
#include "ParkOverview.h"
#include "ResearchManagement.h"
#include "TabbedActions.h"
#include <bit>
#include <openrct2/world/Park.h>
#include <openrct2/management/Marketing.h>

namespace OpenRCT2::Ui::Handheld::ParkManagementTabs
{
    inline void Workspace(ManagementPresentation& m, std::vector<std::string> tabs, size_t tab, size_t row)
    {
        m.tabs=std::move(tabs);m.selectedTab=tab;m.selected=row;
        m.summaryMetrics=true;m.metricHeight=42;
    }
    inline ManagementPresentation Finance(size_t tab, size_t row, FinanceInformation::Ledger ledger,
        size_t graph, size_t sample, const std::string& status={})
    {
        using namespace FinanceInformation;
        auto m=Finances::Presentation(Finances::View::overview,row);
        Workspace(m,{"Overview","Ledger","History","Controls"},tab,row);
        const auto& park=getGameState().park;
        m.actionGrid=true;
        if(tab==0)
        {
            const auto [income,expense]=Finances::MonthlyTotals();
            m.stats={{"Cash",Money(park.cash)},{"Loan balance",Money(park.bankLoan)},
                {"Month net",Money(income==kMoney64Undefined || expense==kMoney64Undefined ? kMoney64Undefined : income-expense)},
                {"Park value",Money(park.value)}};
            m.metricColumns=2;
            m.statInk = { "{WHITE}", "{WHITE}",
                          income == kMoney64Undefined || expense == kMoney64Undefined ? "{WHITE}"
                              : income < expense                                      ? "{DANGER}"
                              : income > expense                                      ? "{POSITIVE}"
                                                                                      : "{WHITE}",
                          "{WHITE}" };
            m.bodyLines={Period(GetDate().GetMonthsElapsed())+" / month to date",
                "Income "+Money(income)+"   Expenses "+Money(expense),
                "Current weekly profit: "+Money(park.currentProfit)};
            m.rows={"Borrow / repay...","Expenditure ledger","Financial history","Financial controls"};
            m.rowEnabled={Finances::Available(Finances::Field::loan),true,true,true};
        }
        else if(tab==1)
        {
            ledger.Clamp();m.title="Expenditure ledger";m.headerStatus=Period(ledger.month);
            m.stats={{"Period net",Money(ledger.Net())}};m.bodyLines.clear();m.rows.clear();m.rowValues.clear();
            for(size_t i=0;i<std::size(kCategories);++i)
            {
                m.rows.push_back(ParkInformation::PlainText(FormatStringIDLegacy(kCategories[i],nullptr),true));
                m.rowValues.push_back(Money(ledger.Value(i)));
            }
            m.actionGrid=false;m.rowHeight=26;m.listLabel="SIGNED CATEGORIES / LEFT-RIGHT MONTH";
            m.primaryAction="Read";m.status="Up/Down entries / Left/Right month";
        }
        else if(tab==2)
        {
            m=History(graph,sample,row);Workspace(m,{"Overview","Ledger","History","Controls"},tab,row);
            m.metricColumns = 2;
            m.reservedHeight = 112;
            m.rows = { "Next financial graph" };
            m.primaryAction="Graph";m.navigationHint="Sample";
        }
        else
        {
            m.title="Financial controls";m.stats={{"Cash",Money(park.cash)},{"Loan",Money(park.bankLoan)},
                {"Loan limit",Money(park.maxBankLoan)},{"Admission",park.flags.has(ParkFlag::noMoney)?"Free":FormatMoney(Park::GetEntranceFee(park))}};
            m.metricColumns=2;m.bodyLines={"Choose a value, adjust its draft, then review."};
            m.rows={"Borrow / repay...","Admission price..."};
            m.rowEnabled={Finances::Available(Finances::Field::loan),Finances::Available(Finances::Field::admission)};
        }
        if(!status.empty())m.status=status;
        return m;
    }
    inline ManagementPresentation Research(size_t tab,size_t row,const std::optional<ResearchManagement::Draft>& draft,
        const std::string& status={})
    {
        auto m=ResearchManagement::Presentation(ResearchManagement::View::overview,row);
        Workspace(m,{"Project","Funding & priorities"},tab,row);
        m.actionGrid=true;
        if(tab==0)
        {
            const auto native=m.stats;
            const auto& s=getGameState();
            const bool finished=s.researchProgressStage==RESEARCH_STAGE_FINISHED_ALL;
            const bool known=s.researchProgressStage<=RESEARCH_STAGE_COMPLETING_DESIGN;
            const int progress=finished?100:known?static_cast<uint32_t>(s.researchProgress)*100/0x10000:-1;
            m.stats={native[3],native[4],{"This stage",progress<0?"Unavailable":std::to_string(progress)+"%"}};
            m.meters={-1,-1,progress};
            m.bodyLines={"Project: "+native[0].second,"Stage: "+native[1].second,"Expected: "+native[2].second};
            m.rows={"Funding & priorities","Invention catalog..."};
            m.rowEnabled={ResearchManagement::FundingAvailable()||ResearchManagement::AvailableCategories(),true};
        }
        else
        {
            const auto d=draft.value_or(ResearchManagement::Begin());
            const auto editor=ResearchManagement::Presentation(ResearchManagement::View::editor,0,d);
            const bool changed=d.funding!=d.baselineFunding||d.priorities!=d.baselinePriorities;
            m.title = "Research plan";
            m.headerStatus = changed ? "Unsaved draft" : "Current settings";
            m.headerInk = changed ? "{CAUTION}" : "{WHITE}";
            m.stats={{"Funding",editor.rows[0].substr(editor.rows[0].find(':')==std::string::npos?0:9)},editor.stats[0]};
            m.metricColumns = 2;
            m.metricHeight = 36;
            m.rowHeight = 24;
            m.contextualAction = "Funding";
            m.bodyLines={"X changes funding / A toggles / Review applies"};
            m.rows.clear();m.rowEnabled.clear();
            for(size_t i=0;i<7;++i)
            {
                const bool available=(ResearchManagement::AvailableCategories()&(1u<<i))!=0;
                auto label=editor.rows[i+1];
                if(!available)label=label.substr(0,label.find("  No projects"))+" / none";
                m.rows.push_back(label);m.rowEnabled.push_back(available);
            }
            m.rows.insert(m.rows.end(),{"Review settings...","Discard draft"});
            m.rowEnabled.insert(m.rowEnabled.end(),{changed,true});m.primaryAction=row<7?"Toggle":"Select";
            m.status = changed ? "Unsaved / B or tab change reviews edits" : "No changes pending";
            const auto query=ResearchManagement::Action(d,false);
            if(query.error!=GameActions::Status::ok)m.status=Finances::Feedback(query);
        }
        if(!status.empty())m.status=status;
        return m;
    }
    inline ManagementPresentation ResearchReview(const ResearchManagement::Draft& d,size_t row,const std::string& status)
    {
        auto m=ResearchManagement::Presentation(ResearchManagement::View::confirmation,row,d,status);
        m.rows={"Apply settings","Cancel"};m.primaryAction="Select";m.bodyLines.clear();
        m.stats.push_back({"Upfront charge",FormatMoney(0)});
        m.bodyLines={"Priorities changed: "+std::to_string(std::popcount(static_cast<unsigned>(d.priorities^d.baselinePriorities))),
            "Funding and selected priorities apply together."};
        return m;
    }
    inline ManagementPresentation Marketing(size_t tab,size_t row,const std::string& status={})
    {
        using namespace MarketingManagement;
        auto m=Presentation(View::overview,row,{},-1,ActiveTypes(),{},status);
        Workspace(m,{"Active campaigns","New campaign"},tab,row);
        m.rows.clear();m.rowDetails.clear();m.rowEnabled.clear();
        if(tab==0)
        {
            m.listLabel = "CAMPAIGN / TARGET / REMAINING WEEKS";
            m.denseRoster = true;
            m.rowHeight = 44;
            for(auto type:ActiveTypes())
            {
                const auto detail=Presentation(View::active,0,{},type);
                m.rows.push_back(Name(type));
                m.rowDetails.push_back(detail.stats[0].second + " / " + detail.stats[1].second + " weeks left");
            }
            if(m.rows.empty())
            {
                m.denseRoster=false;m.bodyLines={"No active campaigns.","R1 opens available campaign types."};
                m.rows={"Choose new campaign"};
            }
        }
        else
        {
            m.title="Choose a campaign";m.actionGrid=true;m.rowHeight=46;
            m.stats={{"Cash",getGameState().park.flags.has(ParkFlag::noMoney)?"Not applicable":FormatMoney(getGameState().park.cash)}};
            for(int type=0;type<ADVERTISING_CAMPAIGN_COUNT;++type)
            {
                m.rows.push_back(Name(type));m.rowEnabled.push_back(Eligible(type));
                const auto reason=MarketingGetCampaign(type)?"Already active":getGameState().park.flags.has(ParkFlag::forbidMarketingCampaigns)?"Forbidden by authority"
                    :!MarketingIsCampaignTypeApplicable(type)?"Park pricing / eligibility"
                    :NeedsTarget(type)&&Targets(type).empty()?"No eligible open target":"From "+FormatMoney(AdvertisingCampaignPricePerWeek[type])+" / week";
                m.rowDetails.push_back(reason);
            }
            m.status="Choose a type / B returns to sections";
        }
        if(!status.empty())m.status=status;
        return m;
    }
    inline ManagementPresentation Park(size_t tab,size_t row,const std::string& status={})
    {
        auto m=ParkOverview::Presentation(row);
        Workspace(m,{"Park","Objectives","Records"},tab,row);
        m.actionGrid=true;
        if(tab==0)
        {
            m.metricColumns=2;m.bodyLines.clear();
            m.rows={m.rows[0],m.rows[1],"Park map...","Read objectives"};
            m.rowEnabled={ParkOverview::StatusPermitted(),Finances::Available(Finances::Field::admission),true,true};
            m.headerStatus=getGameState().park.flags.has(ParkFlag::parkOpen)?"Open":"Closed";
            m.headerInk=getGameState().park.flags.has(ParkFlag::parkOpen)?"{POSITIVE}":"{WHITE}";
        }
        else if(tab==1)
        {
            m.title="Park objectives";m.stats={{"Outcome",ParkOverview::Outcome()}};
            m.rows={"Park map..."};
        }
        else
        {
            m.title="Park records";m.stats={{"Lifetime admissions",std::to_string(getGameState().park.totalAdmissions)},
                {"Admission income",FinanceInformation::Money(getGameState().park.totalIncomeFromAdmissions)}};
            m.bodyLines={"Native news, awards and park history"};m.rows={"Park news...","Park awards...","Park history..."};
        }
        if(!status.empty())m.status=status;
        return m;
    }
}

