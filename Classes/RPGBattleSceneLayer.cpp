//
//  RPGBattleSceneLayer.cpp
//  OzgGameRPG
//
//  Created by ozg on 14-3-18.
//
//

#include "RPGBattleSceneLayer.h"
#include "RPGLoadingSceneLayer.h"
#include "RPGComputingResults.h"

#define MONSTER_QUERY "select * from monster where map_id = %i order by random() limit 1"

RPGBattleSceneLayer::RPGBattleSceneLayer()
{
    
}

RPGBattleSceneLayer::~RPGBattleSceneLayer()
{
    this->m_playerDataList->release();
    this->m_monsterDataList->release();
    this->m_playerList->release();
    this->m_monsterList->release();
        
    CCSpriteFrameCache::sharedSpriteFrameCache()->removeSpriteFramesFromFile("monsters.plist");
    
    CCLog("RPGBattleSceneLayer 释放");
}

bool RPGBattleSceneLayer::init()
{
    if(RPGBaseSceneLayer::init())
    {
        CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("monsters.plist");
        
        //加载语言文件
        string languageFile = CCFileUtils::sharedFileUtils()->fullPathForFilename("scene_battle_cns.plist");
        this->m_stringList = CCDictionary::createWithContentsOfFileThreadSafe(languageFile.c_str());
        this->m_stringList->retain();
        
        CCSprite *bg = CCSprite::create("battle_bg.png");
        bg->setPosition(ccp(CCDirector::sharedDirector()->getWinSize().width / 2, CCDirector::sharedDirector()->getWinSize().height / 2));
        bg->setTag(kRPGBattleSceneLayerTagBg);
        this->addChild(bg);
        
        //背景音乐
        if(SimpleAudioEngine::sharedEngine()->isBackgroundMusicPlaying())
        {
            SimpleAudioEngine::sharedEngine()->stopBackgroundMusic(true);
            SimpleAudioEngine::sharedEngine()->playBackgroundMusic("audio_battle.mp3", true);
        }
        
        //下面的菜单
        CCTMXTiledMap *menuLayer = CCTMXTiledMap::create("battle_menu_style1.tmx");
        menuLayer->setTag(kRPGBattleSceneLayerTagMenuBg);
        menuLayer->setPosition(ccp((CCDirector::sharedDirector()->getWinSize().width - menuLayer->getContentSize().width) / 2.0, (CCDirector::sharedDirector()->getWinSize().height - bg->getContentSize().height) / 2));
        this->addChild(menuLayer);
        
        this->m_playerDataList = new CCArray();
        this->m_playerDataList->init();
        
        this->m_monsterDataList = new CCArray();
        this->m_monsterDataList->init();
        
        this->m_playerList = new CCArray();
        this->m_playerList->init();
        
        this->m_monsterList = new CCArray();
        this->m_monsterList->init();
        
        this->m_speedTotal = 0; //速度总值的初始化
        
        //player
        float playerY = 620;
        float separateY = 197;
        float playerLabY = 215;
        float progressY = 217;
        
        int i = 0;
        
        this->m_playerDataList->removeAllObjects();
        CppSQLite3Query query = this->m_db.execQuery(PLAYER_QUERY);
        while(!query.eof())
        {
            RPGPlayer *playerData = RPGPlayer::create();
            playerData->m_dataId = query.getIntField("id");
            playerData->m_maxHP = query.getIntField("max_hp");
            playerData->m_HP = query.getIntField("hp");
            playerData->m_maxMP = query.getIntField("max_mp");
            playerData->m_MP = query.getIntField("mp");
            playerData->m_attack = query.getFloatField("attack");
            playerData->m_defense = query.getFloatField("defense");
            playerData->m_speed = query.getFloatField("speed");
            playerData->m_skillAttack = query.getFloatField("skill_attack");
            playerData->m_skillDefense = query.getFloatField("skill_defense");
            playerData->m_level = query.getIntField("level");
            playerData->m_name = query.getStringField("name_cns");
            playerData->m_nextExp = query.getIntField("next_exp");
            playerData->m_itemsIdArmor = query.getIntField("items_id_armor");
            playerData->m_itemsIdArms = query.getIntField("items_id_arms");
            playerData->m_skill = query.getStringField("skill");
            playerData->m_texPrefix = query.getStringField("tex_prefix");
            playerData->m_progress = 0.0; //computingProgress方法会重新计算这个值
            
            playerData->m_speed = this->computingFloat(playerData->m_speed);
            this->m_speedTotal += playerData->m_speed; //累加速度总值
            
            this->m_playerDataList->addObject(playerData);
            
            //角色
            RPGBattlePlayerSprite *player = RPGBattlePlayerSprite::createWithPlayerData(playerData);
            player->setPosition(ccp(880, playerY));
            player->setTag(kRPGBattleSceneLayerTagPlayer + playerData->m_dataId);
            this->addChild(player);
            
            this->m_playerList->addObject(player);
            
            //分隔线
            if(i > 0)
            {
                CCSprite *separate = CCSprite::createWithSpriteFrameName("separate.png");
                separate->setPosition(ccp(665, separateY));
                separate->setScaleX(1.4);
                this->addChild(separate);
                
                separateY -= 40;
            }
            
            //名字
            addLab(this, kRPGBattleSceneLayerTagPlayerNameLab + playerData->m_dataId, CCString::create(playerData->m_name), 18, ccp(575, playerLabY));
            
            //进度条
            CCSprite *battleProgressBg = CCSprite::createWithSpriteFrameName("gui_battle_progress_bar01_a.png");
            battleProgressBg->setTag(kRPGBattleSceneLayerTagPlayerProgressBg + playerData->m_dataId);
            battleProgressBg->setPosition(ccp(880, progressY));
            battleProgressBg->setScaleX(0.4);
            battleProgressBg->setScaleY(0.5);
            this->addChild(battleProgressBg);
            
            CCProgressTimer *battleProgress = CCProgressTimer::create(CCSprite::createWithSpriteFrameName("gui_battle_progress_bar01_b.png"));
            battleProgress->setBarChangeRate(ccp(1, 0));    //设置进度条的长度和高度开始变化的大小
            battleProgress->setType(kCCProgressTimerTypeBar);    //设置进度条为水平
            battleProgress->setMidpoint(ccp(0, 0));
            battleProgress->setPosition(ccp(880, progressY));
            battleProgress->setScaleX(0.4);
            battleProgress->setScaleY(0.5);
            battleProgress->setTag(kRPGBattleSceneLayerTagPlayerProgress + playerData->m_dataId);
            this->addChild(battleProgress);
            
            //初始化进度条的进度
//            float progress = this->m_roleListWithInitProgress[role->dataId] * 100;
//            battleProgress->setPercentage(50);
            //CCLog("%f", progress);
            
            //HP
            addLab(this, kRPGBattleSceneLayerTagPlayerHP + playerData->m_dataId, CCString::createWithFormat("%04i / %04i", playerData->m_HP, playerData->m_maxHP), 18, kCCTextAlignmentRight, ccp(420, playerLabY));
            
            //MP
            addLab(this, kRPGBattleSceneLayerTagPlayerMP + playerData->m_dataId, CCString::createWithFormat("%03i / %03i", playerData->m_MP, playerData->m_maxMP), 15, kCCTextAlignmentRight, ccp(535, playerLabY - 3));
            
            playerY -= 100;
            playerLabY -= 40;
            progressY -= 40;
            
            i++;
            query.nextRow();
        }
        query.finalize();
        
        //怪物部分
        
        //第一次调用randomFloat的结果都是一样的，所以不需要
        OzgCCUtility::randomSeed(0);
        OzgCCUtility::randomFloat(0.0f, 1.0f);
        
        //生成怪物数据部分
        int mapId = CCUserDefault::sharedUserDefault()->getIntegerForKey("map_id");

        OzgCCUtility::rangeRand(1, 10);
        int monsterCount = OzgCCUtility::rangeRand(1, 4); //最多出现1到4种怪物
        while (monsterCount > 0)
        {
            //随机查询一个怪物
            CCString *sql = CCString::createWithFormat(MONSTER_QUERY, mapId);
            
            CppSQLite3Query query = this->m_db.execQuery(sql->getCString());
            while(!query.eof())
            {
                RPGMonster *monsterData = RPGMonster::create();
                monsterData->m_dataId = query.getIntField("id");
                monsterData->m_name = query.getStringField("name_cns");
                monsterData->m_maxHP = query.getIntField("max_hp");
                monsterData->m_HP = query.getIntField("max_hp"); //跟上面的一样
                monsterData->m_attack = query.getFloatField("attack");
                monsterData->m_defense = query.getFloatField("defense");
                monsterData->m_speed = query.getFloatField("speed");
                monsterData->m_skillAttack = query.getFloatField("skill_attack");
                monsterData->m_skillDefense = query.getFloatField("skill_defense");
                monsterData->m_exp = query.getIntField("exp");
                monsterData->m_skill = query.getStringField("skill");
                monsterData->m_tex = query.getStringField("tex");
                monsterData->m_progress = 0.0; //computingProgress方法会重新计算这个值
                
                monsterData->m_speed = this->computingFloat(monsterData->m_speed);
                this->m_speedTotal += monsterData->m_speed; //累加速度总值
                
                CCArray *monsters = this->monsterDataListWithDataId(monsterData->m_dataId);
                if(monsters->count() > 0)
                {
                    do
                    {
                        monsterData->m_tag = kRPGBattleSceneLayerTagMonster + OzgCCUtility::rangeRand(0, 9999);
                    }
                    while (this->monsterDataListExistWithTag(monsterData->m_tag));
                    
                    monsters->addObject(monsterData);
                    
//                    CCLog("产生无重复的tag");
                }
                else
                {
                    monsterData->m_tag = kRPGBattleSceneLayerTagMonster + OzgCCUtility::rangeRand(0, 9999);
                    
                    monsters->addObject(monsterData);
                    this->m_monsterDataList->addObject(monsters);
                    
//                    CCLog("如果不存在对应的怪物则直接插入到怪物数组");
                }
                
//                CCLog("产生敌人：%i", monsterData->m_tag);
                
                query.nextRow();
            }
            query.finalize();
            
            monsterCount--;
        }

        //怪物显示部分
        float monsterNameLabY = 220;

        this->m_addedMonsters.clear();
        
        for (i = 0; i < this->m_monsterDataList->count(); i++)
        {
            CCArray *monsters = (CCArray*)this->m_monsterDataList->objectAtIndex(i);
            for (int j = 0; j < monsters->count(); j++)
            {
                RPGMonster *monsterData = (RPGMonster*)monsters->objectAtIndex(j);
                RPGBattleMonsterSprite *monster = RPGBattleMonsterSprite::createWithMonsterData(monsterData);
                this->setMonsterPosition(monster);
                monster->setTag(monsterData->m_tag);
                this->addChild(monster);
                
                this->m_addedMonsters.push_back(monster->getTag());
                
                this->m_monsterList->addObject(monster);
                
                //左下角的怪物名称
                if(j == 0)
                {
                    addLab(this, kRPGBattleSceneLayerTagMonsterNameLab + monsterData->m_dataId, CCString::createWithFormat("%s x %i", monsterData->m_name.c_str(), (int)monsters->count()), 18, ccp(270, monsterNameLabY));
                    
                    monsterNameLabY -= 40;
                }
            }
            
        }
        
        //怪物部分结束
        
        this->m_enabledTouched = false;
        
        this->computingProgress();
                
        this->scheduleUpdate();
        return true;
    }
    return false;
}

CCScene* RPGBattleSceneLayer::scene()
{
    CCScene* s = CCScene::create();
    RPGBattleSceneLayer *layer = RPGBattleSceneLayer::create();
    s->addChild(layer);
    return s;
}

void RPGBattleSceneLayer::update(float delta)
{
    float progressSpeed = 0.5;
    
    //更新player进度条
    for (int i = 0; i < this->m_playerDataList->count(); i++)
    {
        RPGPlayer *playerData = (RPGPlayer*)this->m_playerDataList->objectAtIndex(i);
        playerData->m_progress += progressSpeed;
        
        if(playerData->m_progress > 100)
            playerData->m_progress = 100;
        
        CCProgressTimer *battleProgress = (CCProgressTimer*)this->getChildByTag(kRPGBattleSceneLayerTagPlayerProgress + playerData->m_dataId);
        battleProgress->setPercentage(playerData->m_progress);
        
        if(playerData->m_progress >= 100)
        {
            CCLog("显示player菜单");
            
            RPGBattleMenu *battleMenu = (RPGBattleMenu*)this->getChildByTag(kRPGBattleSceneLayerTagBattleMenu);
            if(battleMenu)
                battleMenu->removeFromParentAndCleanup(true);
            
            battleMenu = RPGBattleMenu::createWithParentNode(this->m_stringList, this, playerData);
            battleMenu->setTag(kRPGBattleSceneLayerTagBattleMenu);
            this->addChild(battleMenu);
            
            this->unscheduleUpdate();
            return;
        }
        
    }
    
    //更新怪物的进度
    for (int i = 0; i < this->m_monsterDataList->count(); i++)
    {
        CCArray *monsters = (CCArray*)this->m_monsterDataList->objectAtIndex(i);
        for (int j = 0; j < monsters->count(); j++)
        {
            RPGMonster *monsterData = (RPGMonster*)monsters->objectAtIndex(j);
//            RPGBattleMonsterSprite *monster = (RPGBattleMonsterSprite*)this->getChildByTag(monsterData->m_tag);
            
            monsterData->m_progress += progressSpeed;
            
            if(monsterData->m_progress > 100)
                monsterData->m_progress = 100;
            
            if(monsterData->m_progress >= 100)
            {
                CCLog("怪物攻击");
                
                this->unscheduleUpdate();
                return;
            }
        }
    }
    
//    CCLog("%f", delta);
}

bool RPGBattleSceneLayer::ccTouchBegan(CCTouch *pTouch, CCEvent *pEvent)
{
    
    return true;
}

void RPGBattleSceneLayer::ccTouchMoved(CCTouch *pTouch, CCEvent *pEvent)
{
    
}

void RPGBattleSceneLayer::ccTouchEnded(CCTouch *pTouch, CCEvent *pEvent)
{
    CCPoint point = pTouch->getLocation();
    
    RPGBattleMenu *battleMenu = (RPGBattleMenu*)this->getChildByTag(kRPGBattleSceneLayerTagBattleMenu);
    
    switch (battleMenu->m_selectedMenuTag)
    {
        case kRPGBattleMenuTagAttack:
        {
//            CCLog("攻击");
            
            SimpleAudioEngine::sharedEngine()->playEffect("audio_effect_btn.wav");
            
            //检测选中的player
            for (int i = 0; i < this->m_playerList->count(); i++)
            {
                RPGBattlePlayerSprite *player = (RPGBattlePlayerSprite*)this->m_playerList->objectAtIndex(i);
                if(player->boundingBox().containsPoint(point))
                {
                    if(player->m_isSelected)
                    {
                        CCLog("攻击player");
                        
                        player->selected(false);
                        this->removeChildByTag(kRPGBattleSceneLayerTagBattleMenu, true);
                        
                    }
                    else
                        player->selected(true);
                }
                else
                    player->selected(false);
                
            }
            
            //检测选中的怪物
            for (int i = 0; i < this->m_monsterList->count(); i++)
            {
                RPGBattleMonsterSprite *monster = (RPGBattleMonsterSprite*)this->m_monsterList->objectAtIndex(i);
                if(monster->boundingBox().containsPoint(point))
                {
                    if(monster->m_isSelected)
                    {
                        CCLog("攻击怪物");
                        
                        RPGBattleMenu *battleMenu = (RPGBattleMenu*)this->getChildByTag(kRPGBattleSceneLayerTagBattleMenu);
                        RPGPlayer *playerData = battleMenu->m_playerData;
                        
                        monster->selected(false);
                        battleMenu->removeFromParentAndCleanup(true);
                        
                        this->attack(playerData, monster->m_data);
                        
                    }
                    else
                        monster->selected(true);
                }
                else
                    monster->selected(false);
                
            }
            
        }
            break;
//        default:
//            break;
    }
    
}

void RPGBattleSceneLayer::showMsg(cocos2d::CCString *msgText)
{
    this->showMsg(msgText, true);
}

void RPGBattleSceneLayer::showMsg(cocos2d::CCString *msgText, bool autoRelease)
{
    CCTMXTiledMap *bgLayer = (CCTMXTiledMap*)this->getChildByTag(kRPGBattleSceneLayerTagMsg);
    
    //之前已经存在了msg框就先释放
    if(bgLayer)
        bgLayer->removeFromParentAndCleanup(true);
    
    //如果正在执行自动释放msg就先停止
    if(this->m_autoReleaseMsg)
    {
        this->m_autoReleaseMsg = false;
        this->unschedule(schedule_selector(RPGBattleSceneLayer::hideMsg));
    }
    
    bgLayer = CCTMXTiledMap::create("battle_msg_style1.tmx");
    bgLayer->setPosition(ccp((CCDirector::sharedDirector()->getWinSize().width - bgLayer->getContentSize().width) / 2, 657));
    bgLayer->setTag(kRPGBattleSceneLayerTagMsg);
    this->addChild(bgLayer);
    
    CCLabelTTF *textLab = CCLabelTTF::create(msgText->getCString(), "Arial", 17, CCSizeMake(400, 25), kCCTextAlignmentCenter, kCCVerticalTextAlignmentCenter);
    textLab->setPosition(ccp(bgLayer->getContentSize().width / 2, bgLayer->getContentSize().height / 2));
    bgLayer->addChild(textLab);
    
    if(autoRelease)
    {
        this->m_autoReleaseMsg = true;
        this->scheduleOnce(schedule_selector(RPGBattleSceneLayer::hideMsg), 3.0);
    }
    else
        this->m_autoReleaseMsg = false;
}

void RPGBattleSceneLayer::hideMsg()
{
    this->hideMsg(0);
}

void RPGBattleSceneLayer::hideMsg(float dt)
{
    CCTMXTiledMap *bgLayer = (CCTMXTiledMap*)this->getChildByTag(kRPGBattleSceneLayerTagMsg);
    
    if(bgLayer)
        bgLayer->removeFromParentAndCleanup(true);
    
    this->m_autoReleaseMsg = false;
}

void RPGBattleSceneLayer::goToMap()
{
    //保存player的数据
    savePlayerData(&this->m_db, this->m_playerDataList);
    
    this->enabledTouched(false);
    
    CCArray *loadTextures = CCArray::create();
    loadTextures->addObject(CCString::create("map.png"));
    loadTextures->addObject(CCString::create("joystick.png"));
    loadTextures->addObject(CCString::create("actor4_0.png"));
    loadTextures->addObject(CCString::create("actor111.png"));
    loadTextures->addObject(CCString::create("actor113.png"));
    loadTextures->addObject(CCString::create("actor114.png"));
    loadTextures->addObject(CCString::create("actor115.png"));
    loadTextures->addObject(CCString::create("actor117.png"));
    loadTextures->addObject(CCString::create("actor120.png"));
    
    CCArray *releaseTextures = CCArray::create();
    releaseTextures->addObject(CCString::create("monsters.png"));
    releaseTextures->addObject(CCString::create("battle_bg.png"));
    
    CCScene *s = RPGLoadingSceneLayer::scene(loadTextures, releaseTextures, "single_map");
    CCTransitionFade *t = CCTransitionFade::create(GAME_SCENE, s);
    CCDirector::sharedDirector()->replaceScene(t);
}

void RPGBattleSceneLayer::enabledTouched(bool enabled)
{
    if(this->m_enabledTouched)
        CCDirector::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
    
    this->m_enabledTouched = enabled;
    
    if(this->m_enabledTouched)
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 1, true);
    
}

void RPGBattleSceneLayer::cancelAllSelected()
{
    for (int i = 0; i < this->m_playerList->count(); i++)
    {
        RPGBattlePlayerSprite *player = (RPGBattlePlayerSprite*)this->m_playerList->objectAtIndex(i);
        player->selected(false);
        
    }
    
    for (int i = 0; i < this->m_monsterList->count(); i++)
    {
        RPGBattleMonsterSprite *monster = (RPGBattleMonsterSprite*)this->m_monsterList->objectAtIndex(i);
        monster->selected(false);
        
    }
    
}

void RPGBattleSceneLayer::attack(cocos2d::CCObject *attackObjData, cocos2d::CCObject *targetObjData)
{
    //发起攻击的对象
    if(dynamic_cast<RPGPlayer*>(attackObjData) != NULL)
    {
        RPGBattlePlayerSprite *player = (RPGBattlePlayerSprite*)this->getChildByTag(kRPGBattleSceneLayerTagPlayer + ((RPGPlayer*)attackObjData)->m_dataId);
        player->animAttack(this, targetObjData);
    }
    else if(dynamic_cast<RPGMonster*>(attackObjData) != NULL)
    {
        
    }
    
}

void RPGBattleSceneLayer::attackResults(cocos2d::CCNode *sender, void *data)
{
    SimpleAudioEngine::sharedEngine()->playEffect("audio_battle_attack2.wav");
    
    //计算伤害值
    
    
}

//private

bool RPGBattleSceneLayer::monsterDataListExistWithTag(int tag)
{
    for (int i = 0; i < this->m_monsterDataList->count(); i++)
    {
        CCArray *monsters = (CCArray*)this->m_monsterDataList->objectAtIndex(i);
        
        for (int j = 0; j < monsters->count(); j++)
        {
            RPGMonster *monster = (RPGMonster*)monsters->objectAtIndex(j);
            if(monster->m_tag == tag)
                return true;
        }
    }
    
    return false;
}

CCArray* RPGBattleSceneLayer::monsterDataListWithDataId(int dataId)
{
    for (int i = 0; i < this->m_monsterDataList->count(); i++)
    {
        RPGMonster *monster = (RPGMonster*)((CCArray*)this->m_monsterDataList->objectAtIndex(i))->objectAtIndex(0);
        if(monster->m_dataId == dataId)
            return (CCArray*)this->m_monsterDataList->objectAtIndex(i);
    }
    
    return CCArray::create();
}

void RPGBattleSceneLayer::setMonsterPosition(RPGBattleMonsterSprite* monster)
{
//    CCLayerColor *l = CCLayerColor::create(ccc4(200, 200, 200, 200), 700, 420);
//    l->setPosition(ccp(50, 270));
//    this->addChild(l);
    
    bool overlap; //是否重叠
    do
    {
        overlap = false;
        
        float x = OzgCCUtility::randomFloat(50 + (monster->getContentSize().width * monster->getAnchorPoint().x), 750 - ((monster->getContentSize().width * monster->getAnchorPoint().x)));
        float y = OzgCCUtility::randomFloat(270 + (monster->getContentSize().height * monster->getAnchorPoint().y), 690 - (monster->getContentSize().height * monster->getAnchorPoint().y));
        
        monster->setPosition(ccp(x, y));
        
        for (int i = 0; i < (int)this->m_addedMonsters.size(); i++)
        {
            RPGBattleMonsterSprite *addedMonster = (RPGBattleMonsterSprite*)this->getChildByTag(this->m_addedMonsters[i]);
            
            if(addedMonster->boundingBox().intersectsRect(monster->boundingBox()))
            {
                overlap = true;
//                CCLog("重新计算怪物的显示位置");
                break;
            }
        }
        
    }
    while (overlap);
    
}

void RPGBattleSceneLayer::computingProgress()
{
//    CCLog("%f", this->m_speedTotal);
    
    //player
    for (int i = 0; i < this->m_playerDataList->count(); i++)
    {
        RPGPlayer *playerData = (RPGPlayer*)this->m_playerDataList->objectAtIndex(i);
        
        CCProgressTimer *battleProgress = (CCProgressTimer*)this->getChildByTag(kRPGBattleSceneLayerTagPlayerProgress + playerData->m_dataId);
        
        float val = playerData->m_speed / this->m_speedTotal * 100.0;
        playerData->m_progress = val;
        battleProgress->setPercentage(playerData->m_progress);
    }
    
    //monster
//    for (int i = 0; i < this->m_monsterDataList->count(); i++)
//    {
//        CCArray *monsters = (CCArray*)this->m_monsterDataList->objectAtIndex(i);
//        for (int j = 0; j < monsters->count(); j++)
//        {
//            RPGMonster *monsterData = (RPGMonster*)monsters->objectAtIndex(j);
//            
//            float val = monsterData->m_speed / this->m_speedTotal * 100.0;
//            monsterData->m_progress = val;
//        }
//    }
    
}

float RPGBattleSceneLayer::computingFloat(float val)
{
    float min = val - (val * 0.1);
    float max = val + (val * 0.1);
    return OzgCCUtility::randomFloat(min, max);
}
