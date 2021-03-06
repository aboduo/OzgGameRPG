//
//  RPGMapStatusMenuLayer.cpp
//  OzgGameRPG
//
//  Created by ozg on 14-4-10.
//
//

#include "RPGMapStatusMenuLayer.h"
#include "RPGMapMenuLayer.h"
#include "RPGMapSceneLayer.h"
#include "OzgCCUtility.h"

//获取一个player的详细数据
#define PLAYER_DETAIL_QUERY "select p.*, arms.name_cns as arms_name, armor.name_cns as armor_name from player as p left join items as arms on p.items_id_arms = arms.id left join items as armor on p.items_id_armor = armor.id where p.id = %i"

//查询一个角色的技能
#define SKILL_QUERY "select * from skill where id in(%s)"

RPGMapStatusMenuLayer::RPGMapStatusMenuLayer()
{

}

RPGMapStatusMenuLayer::~RPGMapStatusMenuLayer()
{
    this->m_stringList->release();
    this->m_skillList->release();
    
//    CCLog("RPGMapStatusMenuLayer 释放");
}

bool RPGMapStatusMenuLayer::init(cocos2d::CCDictionary *stringList, CppSQLite3DB *db, float width, float height)
{
    if(CCLayerColor::initWithColor(ccc4(0, 0, 0, 200), width, height))
    {
        this->m_stringList = stringList;
        this->m_stringList->retain();
        
        this->m_db = db;
        
        this->m_skillList = new CCArray();
        this->m_skillList->init();
        
        this->m_statusIsDefault = true;
        
        CCMenu *mainMenu = CCMenu::create();
        mainMenu->setTag(kRPGMapStatusMenuLayerTagMainMenu);
        mainMenu->setAnchorPoint(CCPointZero);
        mainMenu->setPosition(CCPointZero);
        this->addChild(mainMenu);
        
        CCMenuItemSprite *menuBack = (CCMenuItemSprite*)mainMenu->getChildByTag(kRPGMapStatusMenuLayerTagMainMenuBack);
        if(!menuBack)
        {
            menuBack = CCMenuItemSprite::create(CCSprite::createWithSpriteFrameName("commons_btn_back_04.png"), CCSprite::createWithSpriteFrameName("commons_btn_back_04.png"), this, menu_selector(RPGMapStatusMenuLayer::onMenu));
            menuBack->setPosition(ccp(40, 600));
            menuBack->setTag(kRPGMapStatusMenuLayerTagMainMenuBack);
            menuBack->setScale(0.5);
            mainMenu->addChild(menuBack);
        }
        
        CCTMXTiledMap *mainBg = CCTMXTiledMap::create("map_menu4_style1.tmx");
        mainBg->setPosition(CCPointZero);
        mainBg->setTag(kRPGMapStatusMenuLayerTagBg);
        this->addChild(mainBg);
        
        //显示上面的4个角色
        float playerX = 200;
        CppSQLite3Query query = this->m_db->execQuery(PLAYER_QUERY);
        while(!query.eof())
        {
            CCArray *frames = OzgCCUtility::createSpriteFrames(CCString::createWithFormat("%s_0.png", query.getStringField("tex_prefix"))->getCString(), 3, 4);
            
            CCSprite *playerSprite = CCSprite::createWithSpriteFrame((CCSpriteFrame*)frames->objectAtIndex(1));
            CCMenuItemSprite *player = (CCMenuItemSprite*)mainMenu->getChildByTag(kRPGMapStatusMenuLayerTagMainMenuPlayer + query.getIntField("id"));
            if(!player)
            {
                player = CCMenuItemSprite::create(playerSprite, playerSprite, this, menu_selector(RPGMapStatusMenuLayer::onMenu));
                player->cocos2d::CCNode::setPosition(ccp(playerX, 560));
                player->setScale(2);
                player->setTag(kRPGMapStatusMenuLayerTagMainMenuPlayer + query.getIntField("id"));
                mainMenu->addChild(player);
            }
            
            playerX += 200;
            query.nextRow();
        }
        query.finalize();
        
        //默认选中第一个
        this->onMenu(mainMenu->getChildByTag(kRPGMapStatusMenuLayerTagMainMenuPlayer + 1));
        
        return true;
    }
    return false;
}

RPGMapStatusMenuLayer* RPGMapStatusMenuLayer::create(cocos2d::CCDictionary *stringList, CppSQLite3DB *db, float width, float height)
{
    RPGMapStatusMenuLayer *obj = new RPGMapStatusMenuLayer();
    if(obj && obj->init(stringList, db, width, height))
    {
        obj->autorelease();
        return obj;
    }
    
    CC_SAFE_DELETE(obj);
    return NULL;
}

//private
void RPGMapStatusMenuLayer::onMenu(cocos2d::CCObject *pObject)
{
    //第一次显示数据的话则不播放效果音
    if(!this->m_statusIsDefault)
        SimpleAudioEngine::sharedEngine()->playEffect("audio_effect_btn.wav");
    
    CCMenuItem *menuItem = (CCMenuItem*)pObject;
    
    switch (menuItem->getTag())
    {
        case kRPGMapStatusMenuLayerTagMainMenuBack:
        {
//            CCLog("后退");
            
            //因为动态获取地图的大小会导致了菜单层显示错位，所以定死了
            float width = 960;
            float height = 640;
            RPGMapMenuLayer *menuLayer = menuLayer = RPGMapMenuLayer::create(this->m_stringList, this->m_db, width, height);
            menuLayer->setPosition(ccp((CCDirector::sharedDirector()->getWinSize().width - width) / 2.0, (CCDirector::sharedDirector()->getWinSize().height - height) / 2.0));
            menuLayer->setTag(kRPGMapSceneLayerTagMenuLayer);
            this->getParent()->addChild(menuLayer);
            
            this->removeFromParentAndCleanup(true);
            
        }
            break;
            
        default:
            break;
    }
    
    //player status
    if(menuItem->getTag() >= kRPGMapStatusMenuLayerTagMainMenuPlayer && menuItem->getTag() <= kRPGMapStatusMenuLayerTagMainMenuPlayer + 99)
    {
        CCMenu *mainMenu = (CCMenu*)this->getChildByTag(kRPGMapMenuLayerTagMainMenu);
        
        CppSQLite3Query query = this->m_db->execQuery(PLAYER_QUERY);
        while(!query.eof())
        {
            if(menuItem->getTag() == kRPGMapStatusMenuLayerTagMainMenuPlayer + query.getIntField("id"))
            {
                this->m_statusIsDefault = false;
                
//                CCLog("选中了id为%i的角色", query.getIntField("id"));
                int dataId = query.getIntField("id");
                this->setStatusPlayer(dataId);
                
                //选中的光标部分
                CCMenuItemSprite *player = (CCMenuItemSprite*)mainMenu->getChildByTag(kRPGMapStatusMenuLayerTagMainMenuPlayer + dataId);
                
                CCSprite *handCursor = (CCSprite*)this->getChildByTag(kRPGMapMenuLayerTagHandCursor);
                if(!handCursor)
                {
                    handCursor = CCSprite::createWithSpriteFrameName("gui_hand.png");
                    handCursor->setScale(0.3);
                    handCursor->setTag(kRPGMapMenuLayerTagHandCursor);
                    this->addChild(handCursor);
                }
                
                //光标的位置
                handCursor->setPosition(ccp(player->getPositionX() + 60, 555));
                
                break;
            }
            query.nextRow();
        }
        query.finalize();
    }
    
}

void RPGMapStatusMenuLayer::setStatusPlayer(int dataId)
{
    this->removeAllPlayerLab();
    
    CppSQLite3Query query = this->m_db->execQuery(CCString::createWithFormat(PLAYER_DETAIL_QUERY, dataId)->getCString());
    while(!query.eof())
    {
        //左边
        addLab(this, kRPGMapMenuLayerTagStatusPlayerName, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_name"))->getCString(), query.getStringField("name_cns")), ccp(250, 410));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerLevel, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_level"))->getCString(), query.getIntField("level"), query.getIntField("next_exp")), ccp(243, 370));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerHP, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_hp"))->getCString(), query.getIntField("hp"), query.getIntField("max_hp")), ccp(262, 330));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerMP, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_mp"))->getCString(), query.getIntField("mp"), query.getIntField("max_mp")), ccp(259, 290));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerAttack, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_attack"))->getCString(), query.getIntField("attack")), ccp(249, 250));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerDefense, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_defense"))->getCString(), query.getIntField("defense")), ccp(249, 210));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerSpeed, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_speed"))->getCString(), query.getIntField("speed")), ccp(249, 170));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerSkillAttack, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_skill_attack"))->getCString(), query.getIntField("skill_attack")), ccp(249, 130));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerSkillDefense, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_skill_defense"))->getCString(), query.getIntField("skill_defense")), ccp(249, 90));
        
        //右边装备部分
        addLab(this, kRPGMapMenuLayerTagStatusPlayerEquipArms, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_equip_arms"))->getCString(), query.getStringField("arms_name")), ccp(570, 410));
        addLab(this, kRPGMapMenuLayerTagStatusPlayerEquipArmor, CCString::createWithFormat(((CCString*)this->m_stringList->objectForKey("status_equip_armor"))->getCString(), query.getStringField("armor_name")), ccp(570, 370));
        
        //右边技能部分
        
        //数据处理部分
        this->m_skillList->removeAllObjects();
        string wq = "";
        JsonBox::Value json;
        json.loadFromString(query.getStringField("skill"));
        JsonBox::Array jsonArr = json.getArray();
        for (int i = 0; i < jsonArr.size(); i++)
        {
            char* str = (char*)malloc(10 * sizeof(char));
            OzgCCUtility::itoa(jsonArr[i].getInt(), str);
            wq.append(str);
            
            if(i + 1 < jsonArr.size())
                wq.append(", ");
            
            free(str);
        }
        if((int)wq.length() > 0)
        {
            CppSQLite3Query skillQuery = this->m_db->execQuery(CCString::createWithFormat(SKILL_QUERY, wq.c_str())->getCString());
            while(!skillQuery.eof())
            {
                RPGSkill *skill = RPGSkill::create();
                skill->m_dataId = skillQuery.getIntField("id");
                skill->m_name = skillQuery.getStringField("name_cns");
                skill->m_mp = skillQuery.getIntField("mp");
                skill->m_skillAttack = skillQuery.getIntField("skill_attack");
                
                this->m_skillList->addObject(skill);
                
                skillQuery.nextRow();
            }
            skillQuery.finalize();
        }
        
        CCTableView *tableView = (CCTableView*)this->getChildByTag(kRPGMapStatusMenuLayerTagPlayerSkillTable);
        if(!tableView)
        {
            tableView = CCTableView::create(this, CCSizeMake(600, 265));
            tableView->setDirection(kCCScrollViewDirectionVertical);
            tableView->setPosition(ccp(340, 20));
            tableView->setDelegate(this);
            tableView->setVerticalFillOrder(kCCTableViewFillTopDown);
            tableView->setTag(kRPGMapStatusMenuLayerTagPlayerSkillTable);
            this->addChild(tableView);
        }
        tableView->reloadData();
        
        query.nextRow();
    }
    query.finalize();
    
}

void RPGMapStatusMenuLayer::removeAllPlayerLab()
{
    //清理各个lab部分
    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerName))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerName, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerLevel))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerLevel, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerHP))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerHP, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerMP))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerMP, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerAttack))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerAttack, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerDefense))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerDefense, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerSpeed))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerSpeed, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerSkillAttack))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerSkillAttack, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerSkillDefense))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerSkillDefense, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerEquipArms))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerEquipArms, true);

    if(this->getChildByTag(kRPGMapMenuLayerTagStatusPlayerEquipArmor))
        this->removeChildByTag(kRPGMapMenuLayerTagStatusPlayerEquipArmor, true);
}

//CCTableView
void RPGMapStatusMenuLayer::scrollViewDidScroll(CCScrollView *scrollView)
{
    
}

void RPGMapStatusMenuLayer::scrollViewDidZoom(CCScrollView *scrollView)
{
    
}

void RPGMapStatusMenuLayer::tableCellTouched(CCTableView *tableView, CCTableViewCell *cell)
{
    
}

CCSize RPGMapStatusMenuLayer::cellSizeForTable(CCTableView *tableView)
{
    return CCSizeMake(tableView->getContentSize().width, 80);
}

CCTableViewCell* RPGMapStatusMenuLayer::tableCellAtIndex(CCTableView *tableView, unsigned int idx)
{
    CCTableViewCell *cell = tableView->dequeueCell();
    if (!cell)
    {
        cell = new CCTableViewCell();
        cell->autorelease();
    }
    else
        cell->removeAllChildrenWithCleanup(true);
    
    float x = 150;
    for (int i = 0; i < 3; i++)
    {
        int index = idx * 3 + i;
        
        if(index >= this->m_skillList->count())
            break;
        
        RPGSkill *skillData = (RPGSkill*)this->m_skillList->objectAtIndex(index);
        
        CCLabelTTF *textLab = CCLabelTTF::create(CCString::createWithFormat("%s (%i)", skillData->m_name.c_str(), skillData->m_mp)->getCString(), "Arial", 20, CCSizeMake(200, 25), kCCTextAlignmentLeft);
        textLab->setPosition(ccp(x, 0));
        cell->addChild(textLab);
        
        x += 200;
    }
    
    return cell;
}

unsigned int RPGMapStatusMenuLayer::numberOfCellsInTableView(CCTableView *tableView)
{
    int rowItemCount = 3;
    int size = this->m_skillList->count();
    
    if(size % rowItemCount == 0)
        return size / rowItemCount;
    else
        return size / rowItemCount + 1;
}

//CCTableView end
