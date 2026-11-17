// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/Context.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/drawing/Drawing.Sprite.h>
#include <openrct2/SpriteIds.h>
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
            // Cash is the headline; the loan reads as a bar against its limit; this month and the
            // last eight weeks of profit are a picture rather than three lines of numbers.
            const auto [income,expense]=Finances::MonthlyTotals();
            const bool known=income!=kMoney64Undefined && expense!=kMoney64Undefined;
            m.stats={{"Cash",Money(park.cash)},
                {"Loan, limit "+Money(park.maxBankLoan),Money(park.bankLoan)},
                {"Park value",Money(park.value)},
                {"Weekly profit",Money(park.currentProfit)}};
            m.metricColumns=2;
            m.metricHeight=52;
            m.heroStat=true;
            m.statInk={"{WHITE}","{WHITE}","{WHITE}",park.currentProfit<0?"{DANGER}":"{POSITIVE}"};
            m.meters={-1,park.maxBankLoan>0?static_cast<int>(park.bankLoan*100/park.maxBankLoan):-1,-1,-1};
            ManagementPresentation::Chart chart;
            chart.splitTitle=Period(GetDate().GetMonthsElapsed());
            if(known)
            {
                chart.split={{"In",income,"{POSITIVE}"},{"Out",expense,"{DANGER}"}};
                chart.splitValues={Money(income),Money(expense)};
            }
            chart.columnsTitle="Profit, last 8 weeks";
            for(int week=7;week>=0;--week)
                if(park.weeklyProfitHistory[week]!=kMoney64Undefined)
                    chart.columns.push_back(park.weeklyProfitHistory[week]);
            m.chart=chart;
            m.bodyLines.clear();
            m.rows={"Borrow or repay","Spending ledger"};
            m.rowEnabled={Finances::Available(Finances::Field::loan),true};
            m.primaryRow=0;
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
            m.primaryAction="Read";m.status="< > changes month";
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
            // Admission changes in place with Left/Right (A still opens the exact editor). The loan
            // keeps its review: borrowing is a decision, not a nudge.
            m.title="Financial controls";m.stats={{"Cash",Money(park.cash)},{"Loan",Money(park.bankLoan)},
                {"Loan limit",Money(park.maxBankLoan)}};
            m.metricColumns=3;m.bodyLines.clear();
            const bool admission=Finances::Available(Finances::Field::admission);
            m.rows={"Admission","Borrow / repay..."};
            m.rowEnabled={admission,Finances::Available(Finances::Field::loan)};
            m.settingRows={0};
            m.settingValues={park.flags.has(ParkFlag::noMoney)?"Free":FormatMoney(Park::GetEntranceFee(park))};
            if(row==0&&admission){m.navigationHint="Price";m.primaryAction="Editor";}
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
            // What is being invented, how far along, and funding as a switch changed in place
            // (the monthly cost updates straight away; priorities stay on their own tab).
            ManagementPresentation::Hero hero;
            const bool named=s.researchProgressStage==RESEARCH_STAGE_COMPLETING_DESIGN&&s.researchNextItem&&s.researchNextItem->exists();
            if(named)
            {
                const auto item=*s.researchNextItem;
                hero.picture=[item](Drawing::RenderTarget& rt){
                    const auto type=item.type==Research::EntryType::ride?ObjectType::ride:ObjectType::sceneryGroup;
                    if(const auto* object=GetContext()->GetObjectManager().GetLoadedObject(type,item.entryIndex))
                    {
                        Drawing::RenderTarget inner;
                        if(type==ObjectType::ride&&ClipRenderTarget(inner,rt,{4,2},56,56)){inner.zoom_level=ZoomLevel{1};object->DrawPreview(inner,112,112);}
                        else object->DrawPreview(rt,rt.width,rt.height);
                    }
                };
            }
            else hero.picture=[](Drawing::RenderTarget& rt){GfxDrawSprite(rt,ImageId(SPR_TAB_FINANCES_RESEARCH_0),{20,18});};
            if(ResearchManagement::FundingAvailable())
            {
                hero.states={"None","Minimum","Normal","Maximum"};
                hero.state=std::min<size_t>(s.researchFundingLevel,3);
            }
            hero.meters.push_back({native[1].second,progress<0?"-":std::to_string(progress)+"%",std::max(progress,0)});
            hero.caption=native[0].second;
            m.hero=hero;
            m.stats={native[4],{"Expected",native[2].second}};
            m.metricColumns=2;
            m.bodyLines={"Researching: "+native[0].second};
            m.rows={"Funding","Priorities","Invention catalog..."};
            m.rowEnabled={ResearchManagement::FundingAvailable(),ResearchManagement::FundingAvailable()||ResearchManagement::AvailableCategories()!=0,true};
            m.heroRow=0;
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
            m.bodyLines.clear(); // The footer already says A toggles and X changes funding.
            m.rows.clear();m.rowEnabled.clear();
            for(size_t i=0;i<7;++i)
            {
                const bool available=(ResearchManagement::AvailableCategories()&(1u<<i))!=0;
                auto label=editor.rows[i+1];
                // A category with nothing left to invent is finished, not broken.
                if(!available)label=label.substr(0,label.find("  No projects"))+"  Done";
                m.rows.push_back(label);m.rowEnabled.push_back(available);
            }
            m.rows.insert(m.rows.end(),{"Review settings...","Discard draft"});
            m.rowEnabled.insert(m.rowEnabled.end(),{changed,true});m.primaryAction=row<7?"Toggle":"Select";
            m.status = changed ? "Unsaved changes" : "";
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
            m.status.clear();
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
        else
        {
            // The open/close switch and admission belong to the Park tab; on the others they would
            // be controls that do nothing (and admission's value was landing on another row).
            m.hero.reset();m.heroRow.reset();m.settingRows.clear();m.settingValues.clear();m.bodyLines.clear();
        }
        if(tab==1)
        {
            // What you are playing for: the goal in plain words, how far along you are, and how long
            // is left, which is what the old "Objective: In progress" line never said.
            const auto& gs=getGameState();const auto& park=gs.park;const auto& objective=gs.scenarioOptions.objective;
            using T=Scenario::ObjectiveType;
            m.title="Park objectives";
            const auto outcome=ParkOverview::Outcome();
            m.headerStatus=outcome;
            m.headerInk=outcome=="Completed"?"{POSITIVE}":outcome=="Failed"?"{DANGER}":"{WHITE}";
            ManagementPresentation::Hero hero;
            hero.picture=[](Drawing::RenderTarget& rt){GfxDrawSprite(rt,ImageId(SPR_TAB_OBJECTIVE_0),{18,16});};
            auto meter=[](const std::string& label,money64 current,money64 target,bool money){
                const int percent=target>0?static_cast<int>(std::clamp<money64>(current*100/target,0,100)):0;
                const auto value=money?FinanceInformation::Money(current)+" / "+FinanceInformation::Money(target):std::to_string(current)+" / "+std::to_string(target);
                return ManagementPresentation::Hero::Meter{label,value,percent,current>=target?"{POSITIVE}":"{WHITE}"};
            };
            const auto* last=park.expenditureTable[1];
            switch(objective.Type)
            {
                case T::guestsBy:
                case T::guestsAndRating:
                    hero.meters.push_back(meter("Guests",park.numGuestsInPark,objective.NumGuests,false));
                    hero.meters.push_back(meter("Park rating",park.rating,600,false));
                    break;
                case T::parkValueBy:
                    hero.meters.push_back(meter("Park value",park.value,objective.Currency,true));
                    break;
                case T::repayLoanAndParkValue:
                    hero.meters.push_back(meter("Park value",park.value,objective.Currency,true));
                    hero.meters.push_back({"Loan",park.bankLoan==0?"Repaid":FinanceInformation::Money(park.bankLoan)+" left",park.bankLoan==0?100:0,
                        park.bankLoan==0?"{POSITIVE}":"{WHITE}"});
                    break;
                case T::monthlyRideIncome:
                    hero.meters.push_back(meter("Ride income last month",last[EnumValue(ExpenditureType::parkRideTickets)],objective.Currency,true));
                    break;
                case T::monthlyFoodIncome:
                    hero.meters.push_back(meter("Food and shop profit last month",
                        last[EnumValue(ExpenditureType::shopSales)]+last[EnumValue(ExpenditureType::shopStock)]
                            +last[EnumValue(ExpenditureType::foodDrinkSales)]+last[EnumValue(ExpenditureType::foodDrinkStock)],
                        objective.Currency,true));
                    break;
                default:
                    break;
            }
            if(objective.Type==T::guestsBy||objective.Type==T::parkValueBy)
            {
                const int months=MONTH_COUNT*objective.Year-GetDate().GetMonthsElapsed();
                hero.caption=outcome!="In progress"?outcome:months<=0?"Judged at the end of this month"
                    :months==1?"1 month left":std::to_string(months)+" months left";
            }
            else hero.caption=outcome=="In progress"?"No deadline":outcome;
            m.hero=hero;
            m.stats.clear();
            // Word wrap to the content width, in the menu's own font metrics.
            std::string line;
            for(size_t at=0;;)
            {
                const auto text=ParkOverview::ObjectiveText();
                const auto space=text.find(' ',at);
                const auto word=text.substr(at,space==std::string::npos?std::string::npos:space-at);
                const auto candidate=line.empty()?word:line+" "+word;
                if(!line.empty()&&TextWidth(candidate)>400){m.bodyLines.push_back("{WHITE}"+line);line=word;}
                else line=candidate;
                if(space==std::string::npos)break;
                at=space+1;
            }
            if(!line.empty())m.bodyLines.push_back("{WHITE}"+line);
            m.rows={"Park map..."};
        }
        else if(tab==2)
        {
            const auto& park=getGameState().park;
            m.title="Park records";
            m.stats={{"Lifetime admissions",std::to_string(park.totalAdmissions)},
                {"Admission income",FinanceInformation::Money(park.totalIncomeFromAdmissions)},
                {"Park value",FinanceInformation::Money(park.value)},{"Park rating",std::to_string(park.rating)+" / 999"}};
            m.metricColumns=2;
            m.rows={"Park news...","Park awards...","Park history..."};
        }
        if(!status.empty())m.status=status;
        return m;
    }
}

