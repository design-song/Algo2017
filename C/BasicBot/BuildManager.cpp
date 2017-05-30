#include "BuildManager.h"

using namespace MyBot;

BuildManager::BuildManager() 
{
}
// commit test
// ������� ť�� �ִ� �Ϳ� ���� ���� / �Ǽ� / ����ġ / ���׷��̵带 �����Ѵ�
void BuildManager::update()
{
	// 1��(24������)�� 4�� ������ �����ص� ����ϴ�
	if (BWAPI::Broodwar->getFrameCount() % 6 != 0) return;

	if (buildQueue.isEmpty()) {
		return;
	}

	// Dead Lock �� üũ�ؼ� �����Ѵ�
	checkBuildOrderQueueDeadlockAndAndFixIt();
	// Dead Lock ������ Empty �� �� �ִ�
	if (buildQueue.isEmpty()) {
		return;
	}

	// the current item to be used
	BuildOrderItem currentItem = buildQueue.getHighestPriorityItem();

	//std::cout << "current HighestPriorityItem" << currentItem.metaType.getName() << std::endl;
	
	// while there is still something left in the buildQueue
	while (!buildQueue.isEmpty()) 
	{
		bool isOkToRemoveQueue = true;

		// this is the unit which can produce the currentItem
		BWAPI::Unit producer = getProducer(currentItem.metaType, BWAPI::Position(currentItem.seedLocation), currentItem.producerID);

		/*
		if (currentItem.metaType.isUnit() && currentItem.metaType.getUnitType().isBuilding() ) {
			if (producer) {
				std::cout << "Build " << currentItem.metaType.getName() << " producer : " << producer->getType().getName() << " ID : " << producer->getID() << std::endl;
			}
			else {
				std::cout << "Build " << currentItem.metaType.getName() << " producer nullptr" << std::endl;
			}
		}
		*/

		BWAPI::Unit secondProducer = nullptr;
		bool canMake;

		// �ǹ��� ����� �ִ� ����(�ϲ�)�̳�, ������ ����� �ִ� ����(�ǹ� or ����)�� ������
		if (producer != nullptr) {

			// check to see if we can make it right now
			// ���� �ش� ������ �Ǽ�/���� �� �� �ִ����� ���� �ڿ�, ���ö���, ��ũ Ʈ��, producer ���� ���� �Ǵ��Ѵ�
			canMake = canMakeNow(producer, currentItem.metaType);
			
			/*
			if (currentItem.metaType.isUnit() && currentItem.metaType.getUnitType().isBuilding() ) {
				std::cout << "Build " << currentItem.metaType.getName() << " canMakeNow : " << canMake << std::endl;
			}
			*/

			// �����佺 ���� ���� �� Protoss_Archon / Protoss_Dark_Archon �� ���� Protoss_High_Templar / Protoss_Dark_Templar �� ������ ��ü��Ű�� ����� �Ἥ ����� ������ 
			// secondProducer �� �߰��� ã�� Ȯ���Ѵ�
			if (canMake) {
				if (currentItem.metaType.isUnit()) {
					if (currentItem.metaType.getUnitType() == BWAPI::UnitTypes::Protoss_Archon || currentItem.metaType.getUnitType() == BWAPI::UnitTypes::Protoss_Dark_Archon) {
						secondProducer = getAnotherProducer(producer, producer->getPosition());
						if (secondProducer == nullptr) {
							canMake = false;
						}
					}
				}
			}
		}

		// if we can make the current item, create it
		if (producer != nullptr && canMake == true)
		{			
			MetaType t = currentItem.metaType;

			if (t.isUnit())
			{
				if (t.getUnitType().isBuilding()) {

					// ���� ���� �ǹ� �� Zerg_Lair, Zerg_Hive, Zerg_Greater_Spire, Zerg_Sunken_Colony, Zerg_Spore_Colony �� ���� �ǹ��� Morph ���� �����
					// Morph�� �����ϸ� isMorphing = true, isBeingConstructed = true, isConstructing = true �� �ǰ�
					// �ϼ��Ǹ� isMorphing = false, isBeingConstructed = false, isConstructing = false, isCompleted = true �� �ȴ�
					if (t.getUnitType().getRace() == BWAPI::Races::Zerg && t.getUnitType().whatBuilds().first.isBuilding())
					{
						producer->morph(t.getUnitType());
					}
					// �׶� Addon �ǹ��� ��� (Addon �ǹ��� ������ �ִ����� getProducer �Լ����� �̹� üũ�Ϸ�)
					// ��ǹ��� Addon �ǹ� ���� ������ canBuildAddon = true, isConstructing = false, canCommand = true �̴ٰ� 
					// Addon �ǹ��� ���� �����ϸ� canBuildAddon = false, isConstructing = true, canCommand = true �� �ǰ� (Addon �ǹ� �Ǽ� ��Ҵ� �����ϳ� Train �� Ŀ�ǵ�� �Ұ���)
					// �ϼ��Ǹ� canBuildAddon = false, isConstructing = false �� �ȴ�
					else if (t.getUnitType().isAddon()) {
						
						//std::cout << "addon build start " << std::endl;
						
						producer->buildAddon(t.getUnitType());
						// �׶� Addon �ǹ��� ��� ���������� buildAddon ����� ������ SCV�� ��ǹ� ��ó�� ���� �� �ѵ��� buildAddon ����� ��ҵǴ� ��찡 �־
						// ��ǹ��� isConstructing = true ���·� �ٲ� ���� Ȯ���� �� buildQueue ���� �����ؾ��Ѵ�
						if (producer->isConstructing() == false) {
							isOkToRemoveQueue = false;
						}
						//std::cout << "8";
					}
					// �׿� ��κ� �ǹ��� ���
					else 
					{
						// ConstructionPlaceFinder �� ���� �Ǽ� ���� ��ġ desiredPosition �� �˾Ƴ���
						// ConstructionManager �� ConstructionTask Queue�� �߰��� �ؼ� desiredPosition �� �Ǽ��� �ϰ� �Ѵ�. 
						// ConstructionManager �� �Ǽ� ���߿� �ش� ��ġ�� �Ǽ��� ��������� �ٽ� ConstructionPlaceFinder �� ���� �Ǽ� ���� ��ġ�� desiredPosition �������� ã�� ���̴�
						BWAPI::TilePosition desiredPosition = getDesiredPosition(t.getUnitType(), currentItem.seedLocation, currentItem.seedLocationStrategy);

						// std::cout << "BuildManager "
						//	<< currentItem.metaType.getUnitType().getName().c_str() << " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << std::endl;

						if (desiredPosition != BWAPI::TilePositions::None) {
							// Send the construction task to the construction manager
							ConstructionManager::Instance().addConstructionTask(t.getUnitType(), desiredPosition);
						}
						else {
							// �ǹ� ���� ��ġ�� ���� ����, Protoss_Pylon �� ���ų�, Creep �� ���ų�, Refinery �� �̹� �� �������ְų�, ���� ���� ������ ������ ���� ����ε�,
							// ��κ��� ��� Pylon �̳� Hatchery�� �������� �ִ� ���̹Ƿ�, ���� frame �� �ǹ� ���� ������ �ٽ� Ž���ϵ��� �Ѵ�. 
							std::cout << "There is no place to construct " << currentItem.metaType.getUnitType().getName().c_str()
								<< " strategy " << currentItem.seedLocationStrategy
								<< " seedPosition " << currentItem.seedLocation.x << "," << currentItem.seedLocation.y
								<< " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << std::endl;

							isOkToRemoveQueue = false;
						}
					}
				}
				// �������� / ���������� ���
				else {
					// ���� �������� / ��������
					if (t.getUnitType().getRace() == BWAPI::Races::Zerg)
					{
						// ���� ���� ������ ���� ��κ��� Morph ���� �����
						if (t.getUnitType() != BWAPI::UnitTypes::Zerg_Infested_Terran)
						{
							producer->morph(t.getUnitType());
						}
						// ���� ���� ���� �� Zerg_Infested_Terran �� Train ���� �����
						else {
							producer->train(t.getUnitType());
						}
					}
					// �����佺 �������� / ��������
					else if (t.getUnitType().getRace() == BWAPI::Races::Protoss)
					{						
						// �����佺 ���� ���� �� Protoss_Archon �� ���� Protoss_High_Templar �� ������ ��ü��Ű�� ����� �Ἥ ����� 
						if (t.getUnitType() == BWAPI::UnitTypes::Protoss_Archon)
						{
							producer->useTech(BWAPI::TechTypes::Archon_Warp, secondProducer);
						}
						// �����佺 ���� ���� �� Protoss_Dark_Archon �� ���� Protoss_Dark_Templar �� ������ ��ü��Ű�� ����� �Ἥ ����� 
						else if (t.getUnitType() == BWAPI::UnitTypes::Protoss_Dark_Archon)
						{
							producer->useTech( BWAPI::TechTypes::Dark_Archon_Meld, secondProducer);
						}
						else {
							producer->train(t.getUnitType());
						}
					}
					// �׶� �������� / ��������
					else {
						producer->train(t.getUnitType());
					}
				}
			}
			// if we're dealing with a tech research
			else if (t.isTech())
			{
				producer->research(t.getTechType());
			}
			else if (t.isUpgrade())
			{
				producer->upgrade(t.getUpgradeType());
			}

			//std::cout << std::endl << " build " << t.getName() << " started " << std::endl;
			
			// remove it from the buildQueue
			if (isOkToRemoveQueue) {
				buildQueue.removeCurrentItem();
			}
			
			// don't actually loop around in here
			break;
		}
		// otherwise, if we can skip the current item
		else if (buildQueue.canSkipCurrentItem())
		{
			// skip it and get the next one
			buildQueue.skipCurrentItem();
			currentItem = buildQueue.getNextItem();				
		}
		else 
		{
			// so break out
			break;
		}
	}
}

BWAPI::Unit BuildManager::getProducer(MetaType t, BWAPI::Position closestTo, int producerID)
{
    // get the type of unit that builds this
    BWAPI::UnitType producerType = t.whatBuilds();

    // make a set of all candidate producers
    BWAPI::Unitset candidateProducers;
    for (auto & unit : BWAPI::Broodwar->self()->getUnits())
    {
		if (unit == nullptr) continue;

        // reasons a unit can not train the desired type
        if (unit->getType() != producerType)                    { continue; }
		if (!unit->exists())	                                { continue; }
		if (!unit->isCompleted())                               { continue; }
		if (unit->isTraining())                                 { continue; }
		if (!unit->isPowered())                                 { continue; }
		// if unit is lifted, unit should land first
		if (unit->isLifted())                                   { continue; }

		if (producerID != -1 && unit->getID() != producerID)	{ continue; }
		// TODO : ��ĳ��, Interceptor �� ���, max ��á���� �� �������
        
		// if the type requires an addon and the producer doesn't have one
		typedef std::pair<BWAPI::UnitType, int> ReqPair;
		for (const ReqPair & pair : t.getUnitType().requiredUnits())
		{
			BWAPI::UnitType requiredType = pair.first;
			if (requiredType.isAddon())
			{
				if (!unit->getAddon() || (unit->getAddon()->getType() != requiredType))
				{
					continue;
				}
			}
		}

        // if the type is an addon 
        if (t.getUnitType().isAddon())
        {
            // if the unit already has an addon, it can't make one
            if (unit->getAddon() != nullptr)					{ continue; }

			// ��ǹ��� �Ǽ��ǰ� �ִ� �߿��� isCompleted = false, isConstructing = true, canBuildAddon = false �̴ٰ�
			// �Ǽ��� �ϼ��� �� �� �����ӵ����� isCompleted = true ������, canBuildAddon = false �� ��찡 �ִ�
			if (!unit->canBuildAddon() )						{ continue; }

            // if we just told this unit to build an addon, then it will not be building another one
            // this deals with the frame-delay of telling a unit to build an addon and it actually starting to build
            if (unit->getLastCommand().getType() == BWAPI::UnitCommandTypes::Build_Addon 
                && (BWAPI::Broodwar->getFrameCount() - unit->getLastCommandFrame() < 10)) 
            { 
                continue; 
            }

            bool isBlocked = false;

            // if the unit doesn't have space to build an addon, it can't make one
			BWAPI::TilePosition addonPosition(
				unit->getTilePosition().x + unit->getType().tileWidth(), 
				unit->getTilePosition().y + unit->getType().tileHeight() - t.getUnitType().tileHeight());
            
            for (int i=0; i<t.getUnitType().tileWidth(); ++i)
            {
                for (int j=0; j<t.getUnitType().tileHeight(); ++j)
                {
					BWAPI::TilePosition tilePos(addonPosition.x + i, addonPosition.y + j);

                    // if the map won't let you build here, we can't build it.  
					// �� Ÿ�� ��ü�� �Ǽ� �Ұ����� Ÿ���� ��� + ���� �ǹ��� �ش� Ÿ�Ͽ� �̹� �ִ°��
                    if (!BWAPI::Broodwar->isBuildable(tilePos, true))
                    {
                        isBlocked = true;
                    }

                    // if there are any units on the addon tile, we can't build it
					// �Ʊ� ������ Addon ���� ��ġ�� �־ ������. (���� ������ Addon ���� ��ġ�� ������ �Ǽ� �ȵǴ����� ���� ��Ȯ����)
                    BWAPI::Unitset uot = BWAPI::Broodwar->getUnitsOnTile(tilePos.x, tilePos.y);
					for (auto & u : uot) {
						//std::cout << std::endl << "Construct " << t.getName() 
						//	<< " beside "<< unit->getType().getName() << "(" << unit->getID() <<")" 
						//	<< ", units on Addon Tile " << tilePos.x << "," << tilePos.y << " is " << u->getType().getName() << "(ID : " << u->getID() << " Player : " << u->getPlayer()->getName() << ")" 
						//	<< std::endl;
						if (u->getPlayer() != InformationManager::Instance().selfPlayer) {
							isBlocked = false;
						}
					}					
				}
            }

            if (isBlocked)
            {
                continue;
            }
        }

        // if we haven't cut it, add it to the set of candidates
        candidateProducers.insert(unit);
    }

    return getClosestUnitToPosition(candidateProducers, closestTo);
}

// Protoss_Archon / Protoss_Dark_Archon �� ����� ���� producer �� ���� type��, producer �� �ƴ� �ٸ� Unit �߿��� ���� ����� Unit�� ã�´�
BWAPI::Unit BuildManager::getAnotherProducer(BWAPI::Unit producer, BWAPI::Position closestTo)
{
	if (producer == nullptr) return nullptr;

	BWAPI::Unit closestUnit = nullptr;

	BWAPI::Unitset candidateProducers;
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit == nullptr)									{ continue; }
		if (unit->getType() != producer->getType())             { continue; }
		if (unit->getID() == producer->getID())                 { continue; }
		if (!unit->isCompleted())                               { continue; }
		if (unit->isTraining())                                 { continue; }
		if (!unit->exists())                                    { continue; }
		if (unit->getHitPoints()+unit->getEnergy()<=0)          { continue; }

		candidateProducers.insert(unit);
	}

	return getClosestUnitToPosition(candidateProducers, closestTo);
}

BWAPI::Unit BuildManager::getClosestUnitToPosition(const BWAPI::Unitset & units, BWAPI::Position closestTo)
{
    if (units.size() == 0)
    {
        return nullptr;
    }

    // if we don't care where the unit is return the first one we have
	if (closestTo == BWAPI::Positions::None)
    {
        return *(units.begin());
    }

    BWAPI::Unit closestUnit = nullptr;
    double minDist(1000000000);

	for (auto & unit : units) 
    {
		if (unit == nullptr) continue;

		double distance = unit->getDistance(closestTo);
		if (!closestUnit || distance < minDist) 
        {
			closestUnit = unit;
			minDist = distance;
		}
	}

    return closestUnit;
}

// ���� �ش� ������ �Ǽ�/���� �� �� �ִ����� ���� �ڿ�, ���ö���, ��ũ Ʈ��, producer ���� ���� �Ǵ��Ѵ�
// �ش� ������ �ǹ��� ��� �ǹ� ���� ��ġ�� ���� ���� (Ž���߾��� Ÿ������, �Ǽ� ������ Ÿ������, ������ Pylon�� �ִ���, Creep�� �ִ� ������ ��) �� �Ǵ����� �ʴ´�
bool BuildManager::canMakeNow(BWAPI::Unit producer, MetaType t)
{
	if (producer == nullptr) {
		return false;
	}

	bool canMake = hasEnoughResources(t);

	if (canMake)
	{
		if (t.isUnit())
		{
			// BWAPI::Broodwar->canMake : Checks all the requirements include resources, supply, technology tree, availability, and required units
			canMake = BWAPI::Broodwar->canMake(t.getUnitType(), producer);
		}
		else if (t.isTech())
		{
			canMake = BWAPI::Broodwar->canResearch(t.getTechType(), producer);
		}
		else if (t.isUpgrade())
		{
			canMake = BWAPI::Broodwar->canUpgrade(t.getUpgradeType(), producer);
		}
	}

	return canMake;
}

bool BuildManager::canMake(MetaType t)
{
	bool canMake = false;

	if (canMake)
	{
		if (t.isUnit())
		{
			// Checks all the requirements include resources, supply, technology tree, availability, and required units
			canMake = BWAPI::Broodwar->canMake(t.getUnitType());
		}
		else if (t.isTech())
		{
			canMake = BWAPI::Broodwar->canResearch(t.getTechType());
		}
		else if (t.isUpgrade())
		{
			canMake = BWAPI::Broodwar->canUpgrade(t.getUpgradeType());
		}
	}

	return canMake;

}

// �Ǽ� ���� ��ġ�� ã�´�
// seedLocationStrategy �� SeedPositionSpecified �� ��쿡�� �� ��ó�� ã�ƺ���, SeedPositionSpecified �� �ƴ� ��쿡�� seedLocationStrategy �� ���ݾ� �ٲ㰡�� ��� ã�ƺ���.
// (MainBase -> MainBase ���� -> MainBase ��� -> MainBase ����� �ո��� -> MainBase ����� �ո����� ��� -> �ٸ� ��Ƽ ��ġ -> Ž�� ����)
BWAPI::TilePosition BuildManager::getDesiredPosition(BWAPI::UnitType unitType, BWAPI::TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy)
{
	BWAPI::TilePosition desiredPosition = ConstructionPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(unitType, seedPosition, seedPositionStrategy);

	/*
	 std::cout << "ConstructionPlaceFinder getBuildLocationWithSeedPositionAndStrategy "
		<< unitType.getName().c_str()
		<< " strategy " << seedPositionStrategy
		<< " seedPosition " << seedPosition.x << "," << seedPosition.y
		<< " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << std::endl;
	*/

	// desiredPosition �� ã�� �� ���� ���
	bool findAnotherPlace = true;
	while (desiredPosition == BWAPI::TilePositions::None) {

		switch (seedPositionStrategy) {
		case BuildOrderItem::SeedPositionStrategy::MainBaseLocation:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseBackYard;
			break;
		case BuildOrderItem::SeedPositionStrategy::MainBaseBackYard:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::FirstChokePoint;
			break;
		case BuildOrderItem::SeedPositionStrategy::FirstChokePoint:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation;
			break;
		case BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::SecondChokePoint;
			break;
		case BuildOrderItem::SeedPositionStrategy::SecondChokePoint:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation;
			break;
		case BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation:
		case BuildOrderItem::SeedPositionStrategy::SeedPositionSpecified:
		default:
			findAnotherPlace = false;
			break;
		}

		// �ٸ� ���� �� ã�ƺ���
		if (findAnotherPlace) {
			desiredPosition = ConstructionPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(unitType, seedPosition, seedPositionStrategy);
			/*
			 std::cout << "ConstructionPlaceFinder getBuildLocationWithSeedPositionAndStrategy "
				<< unitType.getName().c_str()
				<< " strategy " << seedPositionStrategy
				<< " seedPosition " << seedPosition.x << "," << seedPosition.y
				<< " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << std::endl;
			*/
		}
		// �ٸ� ���� �� ã�ƺ��� �ʰ�, ������
		else {
			break;
		}
	}

	return desiredPosition;
}

// ��밡�� �̳׶� = ���� ���� �̳׶� - ����ϱ�� ����Ǿ��ִ� �̳׶�
int BuildManager::getAvailableMinerals()
{
	return BWAPI::Broodwar->self()->minerals() - ConstructionManager::Instance().getReservedMinerals();
}

// ��밡�� ���� = ���� ���� ���� - ����ϱ�� ����Ǿ��ִ� ����
int BuildManager::getAvailableGas()
{
	return BWAPI::Broodwar->self()->gas() - ConstructionManager::Instance().getReservedGas();
}

// return whether or not we meet resources, including building reserves
bool BuildManager::hasEnoughResources(MetaType type) 
{
	// return whether or not we meet the resources
	return (type.mineralPrice() <= getAvailableMinerals()) && (type.gasPrice() <= getAvailableGas());
}


// selects a unit of a given type
BWAPI::Unit BuildManager::selectUnitOfType(BWAPI::UnitType type, BWAPI::Position closestTo) 
{
	// if we have none of the unit type, return nullptr right away
	if (BWAPI::Broodwar->self()->completedUnitCount(type) == 0) 
	{
		return nullptr;
	}

	BWAPI::Unit unit = nullptr;

	// if we are concerned about the position of the unit, that takes priority
    if (closestTo != BWAPI::Positions::None) 
    {
		double minDist(1000000000);

		for (auto & u : BWAPI::Broodwar->self()->getUnits()) 
        {
			if (u->getType() == type) 
            {
				double distance = u->getDistance(closestTo);
				if (!unit || distance < minDist) {
					unit = u;
					minDist = distance;
				}
			}
		}

	// if it is a building and we are worried about selecting the unit with the least
	// amount of training time remaining
	} 
    else if (type.isBuilding()) 
    {
		for (auto & u : BWAPI::Broodwar->self()->getUnits()) 
        {
			if (u->getType() == type && u->isCompleted() && !u->isTraining() && !u->isLifted() &&u->isPowered()) {

				return u;
			}
		}
		// otherwise just return the first unit we come across
	} 
    else 
    {
		for (auto & u : BWAPI::Broodwar->self()->getUnits()) 
		{
			if (u->getType() == type && u->isCompleted() && u->getHitPoints() > 0 && !u->isLifted() &&u->isPowered()) 
			{
				return u;
			}
		}
	}

	// return what we've found so far
	return nullptr;
}


BuildManager & BuildManager::Instance()
{
	static BuildManager instance;
	return instance;
}

BuildOrderQueue * BuildManager::getBuildQueue()
{
	return & buildQueue;
}

bool BuildManager::isProducerWillExist(BWAPI::UnitType producerType)
{
	bool isProducerWillExist = true;

	if (BWAPI::Broodwar->self()->completedUnitCount(producerType) == 0
		&& BWAPI::Broodwar->self()->incompleteUnitCount(producerType) == 0)
	{
		// producer �� �ǹ� �� ��� : �ǹ��� �Ǽ� ������ �߰� �ľ�
		// ������� unitType = Addon �ǹ�. Lair. Hive. Greater Spire. Sunken Colony. Spore Colony. �����佺 �� �׶��� �������� / ��������. 
		if (producerType.isBuilding()) {
			if (ConstructionManager::Instance().getConstructionQueueItemCount(producerType) == 0) {
				isProducerWillExist = false;
			}
		}
		// producer �� �ǹ��� �ƴ� ��� : producer �� ������ �������� �߰� �ľ�
		// producerType : �ϲ�. Larva. Hydralisk, Mutalisk 
		else {
			// Larva �� �ð��� ������ Hatchery, Lair, Hive �κ��� �����Ǳ� ������ �ش� �ǹ��� �ִ��� �߰� �ľ�
			if (producerType == BWAPI::UnitTypes::Zerg_Larva)
			{
				if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Hatchery) == 0
					&& BWAPI::Broodwar->self()->incompleteUnitCount(BWAPI::UnitTypes::Zerg_Hatchery) == 0
					&& BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Lair) == 0
					&& BWAPI::Broodwar->self()->incompleteUnitCount(BWAPI::UnitTypes::Zerg_Lair) == 0
					&& BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Hive) == 0
					&& BWAPI::Broodwar->self()->incompleteUnitCount(BWAPI::UnitTypes::Zerg_Hive) == 0)
				{
					if (ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Zerg_Hatchery) == 0
						&& ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Zerg_Lair) == 0
						&& ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Zerg_Hive) == 0)
					{
						isProducerWillExist = false;
					}
				}
			}
			// Hydralisk, Mutalisk �� Egg �κ��� �����Ǳ� ������ �߰� �ľ�
			else if (producerType.getRace() == BWAPI::Races::Zerg) {
				bool isInEgg = false;
				for (auto & unit : BWAPI::Broodwar->self()->getUnits())
				{
					if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg && unit->getBuildType() == producerType) {
						isInEgg = true;
					}
					// ���¾ ������ ���� �ݿ��ȵǾ����� �� �־, �߰� ī��Ʈ�� ������� 
					if (unit->getType() == producerType && unit->isConstructing()) {
						isInEgg = true;
					}
				}
				if (isInEgg == false) {
					isProducerWillExist = false;
				}
			}
			else {
				isProducerWillExist = false;
			}
		}
	}

	return isProducerWillExist;
}

void BuildManager::checkBuildOrderQueueDeadlockAndAndFixIt()
{
	// ��������� ������ �� �ִ� ���������� ���� �Ǵ��Ѵ�
	// this will be true if any unit is on the first frame if it's training time remaining
	// this can cause issues for the build order search system so don't plan a search on these frames
	bool canPlanBuildOrderNow = true;
	for (const auto & unit : BWAPI::Broodwar->self()->getUnits()) {
		if (unit->getRemainingTrainTime() == 0) {
			continue;
		}

		//BWAPI::UnitType trainType = unit->getLastCommand().getUnitType();
		BWAPI::UnitCommand unitCommand = unit->getLastCommand();
		BWAPI::UnitCommandType unitCommandType = unitCommand.getType();
		if (unitCommandType != BWAPI::UnitCommandTypes::None) {
			if (unitCommand.getUnit() != nullptr) {
				BWAPI::UnitType trainType = unitCommand.getUnit()->getType();

				if (unit->getRemainingTrainTime() == trainType.buildTime()) {
					canPlanBuildOrderNow = false;
					break;
				}
			}
		}
	}
	if (!canPlanBuildOrderNow) {
		return;
	}

	// BuildQueue �� HighestPriority �� �ִ� BuildQueueItem �� skip �Ұ����� ���ε�, ���������� ������ �� ���ų�, ������ �����ε� ��� �Ұ����� ���, dead lock �� �߻��Ѵ�
	// ���� �ǹ��� BuildQueue�� �߰��س�����, �ش� BuildQueueItem �� �������� ���������� �Ǵ��ؾ� �Ѵ�
	BuildOrderQueue * buildQueue = BuildManager::Instance().getBuildQueue();
	if (!buildQueue->isEmpty())
	{
		BuildOrderItem currentItem = buildQueue->getHighestPriorityItem();

		// TODO : ����. canSkipCurrentItem �����ִ°�?
		//if (buildQueue->canSkipCurrentItem() == false)
		if (currentItem.blocking == true)
		{
			bool isDeadlockCase = false;

			// producerType�� ���� �˾Ƴ���
			BWAPI::UnitType producerType = currentItem.metaType.whatBuilds();

			// �ǹ��̳� ������ ���
			if (currentItem.metaType.isUnit())
			{
				BWAPI::UnitType unitType = currentItem.metaType.getUnitType();
				BWAPI::TechType requiredTechType = unitType.requiredTech();
				const std::map< BWAPI::UnitType, int >& requiredUnits = unitType.requiredUnits();
				int requiredSupply = unitType.supplyRequired();

				/*
				std::cout << "To make " << unitType.getName()
				<< ", producerType " << producerType.getName()
				<< " completedUnitCount " << BWAPI::Broodwar->self()->completedUnitCount(producerType)
				<< " incompleteUnitCount " << BWAPI::Broodwar->self()->incompleteUnitCount(producerType)
				<< std::endl;
				*/

				// �ǹ��� �����ϴ� �����̳�, ������ �����ϴ� �ǹ��� �������� �ʰ�, �Ǽ� ���������� ������ dead lock
				if (isProducerWillExist(producerType) == false) {
					isDeadlockCase = true;
				}

				// Refinery �ǹ��� ���, Refinery �� �Ǽ����� ���� Geyser�� �ִ� ��쿡�� ����
				if (!isDeadlockCase && unitType == InformationManager::Instance().getRefineryBuildingType())
				{
					bool hasAvailableGeyser = true;

					// Refinery�� ������ �� �ִ� ��Ҹ� ã�ƺ���
					BWAPI::TilePosition testLocation = getDesiredPosition(unitType, currentItem.seedLocation, currentItem.seedLocationStrategy);
					
					// Refinery �� �������� ��Ҹ� ã�� �� ������ dead lock
					if (testLocation == BWAPI::TilePositions::None || testLocation == BWAPI::TilePositions::Invalid || testLocation.isValid() == false) {
						std::cout << "Build Order Dead lock case -> Cann't find place to construct " << unitType.getName() << std::endl;
						hasAvailableGeyser = false;
					}
					else {
						// Refinery �� �������� ��ҿ� Refinery �� �̹� �Ǽ��Ǿ� �ִٸ� dead lock 
						BWAPI::Unitset uot = BWAPI::Broodwar->getUnitsOnTile(testLocation);
						for (auto & u : uot) {
							if (u->getType().isRefinery() && u->exists()) {
								hasAvailableGeyser = false;
								break;
							}
						}
					}

					if (hasAvailableGeyser == false) {
						isDeadlockCase = true;
					}
				}
				
				// ���� ��� ����ġ�� �Ǿ����� �ʰ�, ����ġ �������� ������ dead lock
				if (!isDeadlockCase && requiredTechType != BWAPI::TechTypes::None)
				{
					if (BWAPI::Broodwar->self()->hasResearched(requiredTechType) == false) {
						if (BWAPI::Broodwar->self()->isResearching(requiredTechType) == false) {
							isDeadlockCase = true;
						}
					}
				}

				// ���� �ǹ�/������ �ִµ� 
				if (!isDeadlockCase && requiredUnits.size() > 0)
				{
					for (auto & u : requiredUnits)
					{
						BWAPI::UnitType requiredUnitType = u.first;

						if (requiredUnitType != BWAPI::UnitTypes::None) {

							/*
							std::cout << "pre requiredUnitType " << requiredUnitType.getName()
							<< " completedUnitCount " << BWAPI::Broodwar->self()->completedUnitCount(requiredUnitType)
							<< " incompleteUnitCount " << BWAPI::Broodwar->self()->incompleteUnitCount(requiredUnitType)
							<< std::endl;
							*/

							// ���� �ǹ� / ������ �������� �ʰ�, ���� �������� �ʰ�
							if (BWAPI::Broodwar->self()->completedUnitCount(requiredUnitType) == 0
								&& BWAPI::Broodwar->self()->incompleteUnitCount(requiredUnitType) == 0)
							{
								// ���� �ǹ��� �Ǽ� ���������� ������ dead lock
								if (requiredUnitType.isBuilding())
								{
									if (ConstructionManager::Instance().getConstructionQueueItemCount(requiredUnitType) == 0) {
										isDeadlockCase = true;
									}
								}
								// ���� ������ Larva �� Zerg ������ ���, Larva, Hatchery, Lair, Hive �� �ϳ��� �������� �ʰ�, �Ǽ� �������� ���� ��쿡 dead lock
								else if (requiredUnitType == BWAPI::UnitTypes::Zerg_Larva)
								{
									if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Hatchery) == 0
										&& BWAPI::Broodwar->self()->incompleteUnitCount(BWAPI::UnitTypes::Zerg_Hatchery) == 0
										&& BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Lair) == 0
										&& BWAPI::Broodwar->self()->incompleteUnitCount(BWAPI::UnitTypes::Zerg_Lair) == 0
										&& BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Hive) == 0
										&& BWAPI::Broodwar->self()->incompleteUnitCount(BWAPI::UnitTypes::Zerg_Hive) == 0)
									{
										if (ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Zerg_Hatchery) == 0
											&& ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Zerg_Lair) == 0
											&& ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Zerg_Hive) == 0)
										{
											isDeadlockCase = true;
										}
									}
								}
							}
						}
					}
				}

				// �ǹ��� �ƴ� ����/���� ������ ���, ���ö��̰� 400 �� á���� dead lock
				if (!isDeadlockCase && !unitType.isBuilding()
					&& BWAPI::Broodwar->self()->supplyTotal() == 400 && BWAPI::Broodwar->self()->supplyUsed() + unitType.supplyRequired() > 400)
				{
					isDeadlockCase = true;
				}

				// �ǹ��� �ƴ� ����/���� ������ ���, ���ö��̰� �����ϸ� dead lock ������, ���ö��� �����ϴٰ� ����/�������� ���带 ����ϱ⺸�ٴ� ���� ���ö��̸� ������ �ϱ� ����, 
				// �̰��� StrategyManager ��� ���� ó���ϵ��� �Ѵ� 

				// Pylon �� �ʿ��� �ǹ��� ���, �ش� ���� ������ ���� �������� �ϴµ�, Pylon �� �ش� ���� ������ ������ dead lock
				// Pylon ������ ��Ȯ�ϰ� �ľ��ϰų�  ������� �����ΰ�, �Ǽ��������� ���� �ϴ� üũ���� �ʴ´�
				if (!isDeadlockCase && unitType.isBuilding() && unitType.requiresPsi() 
					&& currentItem.seedLocationStrategy == BuildOrderItem::SeedPositionStrategy::SeedPositionSpecified)
				{
					bool hasFoundPylon = false;
					BWAPI::Unitset ourUnits = BWAPI::Broodwar->getUnitsInRadius(BWAPI::Position(currentItem.seedLocation), 4 * TILE_SIZE);

					for (auto & u : ourUnits) {
						if (u->getPlayer() == BWAPI::Broodwar->self() 
							&& u->getType() == BWAPI::UnitTypes::Protoss_Pylon) {
							hasFoundPylon = true;
						}
					}

					if (hasFoundPylon == false) {
						isDeadlockCase = true;
					}
				}

				// Creep �� �ʿ��� �ǹ��� ���, �ش� ���� ������ Hatchery�� Creep Colony ���� �־�� �ϴµ�, �ش� ���� ������ ������ dead lock
				// Creep ������ ��Ȯ�ϰ� �ľ��ϰų�  ������� �����ΰ�, �Ǽ��������� ���� �ϴ� üũ���� �ʴ´�
				if (!isDeadlockCase && unitType.isBuilding() && unitType.requiresCreep()
					&& currentItem.seedLocationStrategy == BuildOrderItem::SeedPositionStrategy::SeedPositionSpecified)
				{
					bool hasFoundCreepGenerator = false;
					BWAPI::Unitset ourUnits = BWAPI::Broodwar->getUnitsInRadius(BWAPI::Position(currentItem.seedLocation), 4 * TILE_SIZE);

					for (auto & u : ourUnits) {
						if (u->getPlayer() == BWAPI::Broodwar->self()
							&& (u->getType() == BWAPI::UnitTypes::Zerg_Hatchery || u->getType() == BWAPI::UnitTypes::Zerg_Lair || u->getType() == BWAPI::UnitTypes::Zerg_Hive
								|| u->getType() == BWAPI::UnitTypes::Zerg_Creep_Colony || u->getType() == BWAPI::UnitTypes::Zerg_Sunken_Colony || u->getType() == BWAPI::UnitTypes::Zerg_Spore_Colony) )
						{
							hasFoundCreepGenerator = true;
						}
					}

					if (hasFoundCreepGenerator == false) {
						isDeadlockCase = true;
					}
				}

			}
			// ��ũ�� ���, �ش� ����ġ�� �̹� �߰ų�, �̹� �ϰ��ְų�, ����ġ�� �ϴ� �ǹ� �� ����ǹ��� ���������ʰ� �Ǽ����������� ������ dead lock
			else if (currentItem.metaType.isTech())
			{
				BWAPI::TechType techType = currentItem.metaType.getTechType();
				BWAPI::UnitType requiredUnitType = techType.requiredUnit();

				/*
				std::cout << "To research " << techType.getName()
				<< ", hasResearched " << BWAPI::Broodwar->self()->hasResearched(techType)
				<< ", isResearching " << BWAPI::Broodwar->self()->isResearching(techType)
				<< ", producerType " << producerType.getName()
				<< " completedUnitCount " << BWAPI::Broodwar->self()->completedUnitCount(producerType)
				<< " incompleteUnitCount " << BWAPI::Broodwar->self()->incompleteUnitCount(producerType)
				<< std::endl;
				*/

				if (BWAPI::Broodwar->self()->hasResearched(techType) || BWAPI::Broodwar->self()->isResearching(techType)) {
					isDeadlockCase = true;
				}
				else if (BWAPI::Broodwar->self()->completedUnitCount(producerType) == 0
					&& BWAPI::Broodwar->self()->incompleteUnitCount(producerType) == 0)
				{
					if (ConstructionManager::Instance().getConstructionQueueItemCount(producerType) == 0) {

						// ��ũ ����ġ�� producerType�� Addon �ǹ��� ���, Addon �ǹ� �Ǽ��� ��� ���������� ���۵Ǳ� �������� getUnits, completedUnitCount, incompleteUnitCount ���� Ȯ���� �� ����
						// producerType�� producerType �ǹ��� ���� Addon �ǹ� �Ǽ��� ����� ���������� Ȯ���ؾ� �Ѵ�
						if (producerType.isAddon()) {

							bool isAddonConstructing = false;

							BWAPI::UnitType producerTypeOfProducerType = producerType.whatBuilds().first;

							if (producerTypeOfProducerType != BWAPI::UnitTypes::None) {

								for (auto & unit : BWAPI::Broodwar->self()->getUnits())
								{
									if (unit == nullptr) continue;
									if (unit->getType() != producerTypeOfProducerType)	{ continue; }

									// ��ǹ��� �ϼ��Ǿ��ְ�, ��ǹ��� �ش� Addon �ǹ��� �Ǽ������� Ȯ���Ѵ�
									if (unit->isCompleted() && unit->isConstructing() && unit->getBuildType() == producerType) {
										isAddonConstructing = true;
										break;
									}
								}
							}

							if (isAddonConstructing == false) {
								isDeadlockCase = true;
							}
						}
						else {
							isDeadlockCase = true;
						}
					}
				}
				else if (requiredUnitType != BWAPI::UnitTypes::None) {
					/*
					std::cout << "To research " << techType.getName()
					<< ", requiredUnitType " << requiredUnitType.getName()
					<< " completedUnitCount " << BWAPI::Broodwar->self()->completedUnitCount(requiredUnitType)
					<< " incompleteUnitCount " << BWAPI::Broodwar->self()->incompleteUnitCount(requiredUnitType)
					<< std::endl;
					*/

					if (BWAPI::Broodwar->self()->completedUnitCount(requiredUnitType) == 0
						&& BWAPI::Broodwar->self()->incompleteUnitCount(requiredUnitType) == 0) {
						if (ConstructionManager::Instance().getConstructionQueueItemCount(requiredUnitType) == 0) {
							isDeadlockCase = true;
						}
					}
				}
			}
			// ���׷��̵��� ���, �ش� ���׷��̵带 �̹� �߰ų�, �̹� �ϰ��ְų�, ���׷��̵带 �ϴ� �ǹ� �� ����ǹ��� ���������� �ʰ� �Ǽ����������� ������ dead lock
			else if (currentItem.metaType.isUpgrade())
			{
				BWAPI::UpgradeType upgradeType = currentItem.metaType.getUpgradeType();
				int maxLevel = BWAPI::Broodwar->self()->getMaxUpgradeLevel(upgradeType);
				int currentLevel = BWAPI::Broodwar->self()->getUpgradeLevel(upgradeType);
				BWAPI::UnitType requiredUnitType = upgradeType.whatsRequired();

				/*
				std::cout << "To upgrade " << upgradeType.getName()
				<< ", maxLevel " << maxLevel
				<< ", currentLevel " << currentLevel
				<< ", isUpgrading " << BWAPI::Broodwar->self()->isUpgrading(upgradeType)
				<< ", producerType " << producerType.getName()
				<< " completedUnitCount " << BWAPI::Broodwar->self()->completedUnitCount(producerType)
				<< " incompleteUnitCount " << BWAPI::Broodwar->self()->incompleteUnitCount(producerType)
				<< ", requiredUnitType " << requiredUnitType.getName()
				<< std::endl;
				*/

				if (currentLevel >= maxLevel || BWAPI::Broodwar->self()->isUpgrading(upgradeType)) {
					isDeadlockCase = true;
				}
				else if (BWAPI::Broodwar->self()->completedUnitCount(producerType) == 0
					&& BWAPI::Broodwar->self()->incompleteUnitCount(producerType) == 0) {
					if (ConstructionManager::Instance().getConstructionQueueItemCount(producerType) == 0) {

						// ���׷��̵��� producerType�� Addon �ǹ��� ���, Addon �ǹ� �Ǽ��� ���۵Ǳ� �������� getUnits, completedUnitCount, incompleteUnitCount ���� Ȯ���� �� ����
						// producerType�� producerType �ǹ��� ���� Addon �ǹ� �Ǽ��� ���۵Ǿ��������� Ȯ���ؾ� �Ѵ�						
						if (producerType.isAddon()) {

							bool isAddonConstructing = false;

							BWAPI::UnitType producerTypeOfProducerType = producerType.whatBuilds().first;

							if (producerTypeOfProducerType != BWAPI::UnitTypes::None) {

								for (auto & unit : BWAPI::Broodwar->self()->getUnits())
								{
									if (unit == nullptr) continue;
									if (unit->getType() != producerTypeOfProducerType)	{ continue; }
									// ��ǹ��� �ϼ��Ǿ��ְ�, ��ǹ��� �ش� Addon �ǹ��� �Ǽ������� Ȯ���Ѵ�
									if (unit->isCompleted() && unit->isConstructing() && unit->getBuildType() == producerType) {
										isAddonConstructing = true;
										break;
									}
								}
							}

							if (isAddonConstructing == false) {
								isDeadlockCase = true;
							}
						}
						else {
							isDeadlockCase = true;
						}
					}
				}
				else if (requiredUnitType != BWAPI::UnitTypes::None) {
					if (BWAPI::Broodwar->self()->completedUnitCount(requiredUnitType) == 0
						&& BWAPI::Broodwar->self()->incompleteUnitCount(requiredUnitType) == 0) {
						if (ConstructionManager::Instance().getConstructionQueueItemCount(requiredUnitType) == 0) {
							isDeadlockCase = true;
						}
					}
				}
			}

			if (isDeadlockCase) {
				std::cout << std::endl << "Build Order Dead lock case -> remove BuildOrderItem " << currentItem.metaType.getName() << std::endl;

				buildQueue->removeCurrentItem();
			}

		}
	}

}



