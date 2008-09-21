/************************************************************************************

    Copyright (C) 2000-2002, 2007 Thibaut Tollemer
    Copyright (C) 2008 Markus Drescher

    This file is part of Bombermaaan.

    Bombermaaan is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Bombermaaan is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Bombermaaan.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************************/


//////////////////
// CAiBomber.cpp

#include "STDAFX.H"
#include "CAiArena.h"
#include "CAiBomber.h"

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

#define AI_VIEW_SIZE        6

int CAiBomber::m_BurnMark[4][6] =
{
    {  0,  0,  0,  0,  0,  0 },
    { 10,  8,  5,  3,  2,  1 },
    { 20, 17, 15, 12, 10,  5 },
    { 30, 26, 24, 22,  5, 10 }
};

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

CAiBomber::CAiBomber (void)
{
    m_pArena = NULL;
    m_Player = -1;
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

CAiBomber::~CAiBomber (void)
{
    
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

void CAiBomber::Create (int Player)
{
    ASSERT (m_pArena != NULL);
    m_Player = Player;

    // Wait a little before thinking for the first time
    m_StopTimeLeft = 0.1f;

    // Reset commands variables
    m_BomberMove = BOMBERMOVE_NONE;
    m_BomberAction = BOMBERACTION_NONE;
    m_BomberMoveTimeLeft = 0.0f;

    m_BlockWalk = 0;

    // Think next time
    m_ComputerMode = COMPUTERMODE_THINK; // added due to a valgrind warning
    SetComputerMode (COMPUTERMODE_THINK);
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

void CAiBomber::Destroy (void)
{
}


//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

void CAiBomber::Update (float DeltaTime)
{
    // Pointer to bomber
    m_pBomber = &m_pArena->m_pArena->GetBomber (m_Player);

    // Think and send commands to the bomber only if the bomber is alive
    if (m_pBomber->IsAlive())
    {    
        // If the player does not have to stop commanding his bomber
        if (m_StopTimeLeft <= 0.0f)
        {
            // If the current bombermove's duration has elapsed
            if (m_BomberMoveTimeLeft <= 0.0f)
            {
                m_BlockWalk++;

                // Bomber block coordinates
                m_BlockHereX = m_pBomber->GetBlockX ();
                m_BlockHereY = m_pBomber->GetBlockY ();

                // Coordinates of blocks around the bomber
                m_BlockUpX    = m_BlockHereX;
                m_BlockUpY    = m_BlockHereY - 1;
                m_BlockDownX  = m_BlockHereX;
                m_BlockDownY  = m_BlockHereY + 1;
                m_BlockLeftX  = m_BlockHereX - 1;
                m_BlockLeftY  = m_BlockHereY;
                m_BlockRightX = m_BlockHereX + 1;
                m_BlockRightY = m_BlockHereY;

                UpdateAccessibility();

                // If the AI has to think
                if (m_ComputerMode == COMPUTERMODE_THINK)
                {
                    // Think right now. We do this because the AI should ACT
                    // just after deciding what to do. If we didn't do this,
                    // the bomber would have no commands sent and its sprite
                    // animation would be affected (and it looks bad).
                    ModeThink();
                }

                // Update the computer player according to its mode
                switch (m_ComputerMode)
                {                
                    case COMPUTERMODE_ITEM    : ModeItem (DeltaTime);    break;
                    case COMPUTERMODE_ATTACK  : ModeAttack ();           break;
                    case COMPUTERMODE_DEFENCE : ModeDefence (DeltaTime); break;
                    case COMPUTERMODE_WALK    : ModeWalk (DeltaTime);    break;
                    default: break;
                }

                if (m_pBomber->GetSickness() == SICK_INVERTION)
                {
                    switch (m_BomberMove)
                    {
                        case BOMBERMOVE_UP    : m_BomberMove = BOMBERMOVE_DOWN;  break;
                        case BOMBERMOVE_DOWN  : m_BomberMove = BOMBERMOVE_UP;    break;
                        case BOMBERMOVE_LEFT  : m_BomberMove = BOMBERMOVE_RIGHT; break;
                        case BOMBERMOVE_RIGHT : m_BomberMove = BOMBERMOVE_LEFT;  break;
                        default: break;
                    }
                }
            }

            // Send commands to the bomber
            m_pBomber->Command (m_BomberMove, m_BomberAction);
    
            // Decrease time left before the bombermove has to be updated
            m_BomberMoveTimeLeft -= DeltaTime;
        }
        // If the player has to stop commanding his bomber
        else
        {
            // Send no command to the bomber
            m_pBomber->Command (BOMBERMOVE_NONE, BOMBERACTION_NONE);
    
            // Decrease time left before sending commands to the bomber
            m_StopTimeLeft -= DeltaTime;
        }
    }
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

bool CAiBomber::EnemyNearAndFront (void)
{
    int BlockX;
    int BlockY;

    if (m_pArena->m_pArena->IsBomb (m_BlockHereX, m_BlockHereY))
    {
        return false;
    }

    //-----------------------------------------
    // Check if there is an enemy where we are
    //-----------------------------------------

    // Scan the players
    for (int Index = 0 ; Index < MAX_PLAYERS ; Index++)
    {
        // If the current player is not the one we are controlling
        // and the bomber of this player exists and is alive
        // and this bomber is where our bomber is
        if (Index != m_Player &&
            m_pArena->m_pArena->GetBomber(Index).Exist() && 
            m_pArena->m_pArena->GetBomber(Index).IsAlive() && 
            m_pArena->m_pArena->GetBomber(Index).GetBlockX() == m_BlockHereX && 
            m_pArena->m_pArena->GetBomber(Index).GetBlockY() == m_BlockHereY)
        {
            // There is an enemy near our bomber
            return true;
        }
    }

    #define MAX_NEAR_DISTANCE       3
        
    //---------------------------------------------------------
    // Check if there is an enemy near our bomber to the right
    //---------------------------------------------------------

    // Start scanning one block to the right
    BlockX = m_BlockHereX + 1;
    BlockY = m_BlockHereY;

    // While we are still near our bomber
    while (BlockX <= m_BlockHereX + MAX_NEAR_DISTANCE)
    {
        // If we are scanning out of the arena
        // or if there is a wall or a bomb where we are scanning
        if (BlockX >= ARENA_WIDTH ||
            m_pArena->m_pArena->IsWall (BlockX, BlockY) ||
            m_pArena->m_pArena->IsBomb (BlockX, BlockY))
        {
            // Stop scanning, there is no enemy near and in front of our bomber in this direction
            break;
        }

        // If there is a bomber where we are scanning
        if (m_pArena->m_pArena->IsBomber (BlockX, BlockY))
        {
            // We have an enemy bomber to the right that is near
            // and in front of our bomber.
            return true;
        }

        // Continue scanning (go right)
        BlockX++;
    }

    //---------------------------------------------------------
    // Check if there is an enemy near our bomber to the left
    //---------------------------------------------------------

    // Start scanning one block to the left
    BlockX = m_BlockHereX - 1;
    BlockY = m_BlockHereY;

    // While we are still near our bomber
    while (BlockX >= m_BlockHereX - MAX_NEAR_DISTANCE)
    {
        // If we are scanning out of the arena
        // or if there is a wall or a bomb where we are scanning
        if (BlockX < 0 ||
            m_pArena->m_pArena->IsWall (BlockX, BlockY) ||
            m_pArena->m_pArena->IsBomb (BlockX, BlockY))
        {
            // Stop scanning, there is no enemy near and in front of our bomber in this direction
            break;
        }

        // If there is a bomber where we are scanning
        if (m_pArena->m_pArena->IsBomber (BlockX, BlockY))
        {
            // We have an enemy bomber to the left that is near
            // and in front of our bomber.
            return true;
        }

        // Continue scanning (go left)
        BlockX--;
    }

    //---------------------------------------------------------
    // Check if there is an enemy near our bomber above
    //---------------------------------------------------------

    // Start scanning one block above
    BlockX = m_BlockHereX;
    BlockY = m_BlockHereY - 1;

    // While we are still near our bomber
    while (BlockY >= m_BlockHereY - MAX_NEAR_DISTANCE)
    {
        // If we are scanning out of the arena
        // or if there is a wall or a bomb where we are scanning
        if (BlockY < 0 ||
            m_pArena->m_pArena->IsWall (BlockX, BlockY) ||
            m_pArena->m_pArena->IsBomb (BlockX, BlockY))
        {
            // Stop scanning, there is no enemy near and in front of our bomber in this direction
            break;
        }

        // If there is a bomber where we are scanning
        if (m_pArena->m_pArena->IsBomber (BlockX, BlockY))
        {
            // We have an enemy bomber above that is near
            // and in front of our bomber.            
            return true;
        }

        // Continue scanning (go up)
        BlockY--;
    }

    //---------------------------------------------------------
    // Check if there is an enemy near our bomber below
    //---------------------------------------------------------

    // Start scanning one block below
    BlockX = m_BlockHereX;
    BlockY = m_BlockHereY + 1;

    // While we are still near our bomber
    while (BlockY <= m_BlockHereY + MAX_NEAR_DISTANCE)
    {
        // If we are scanning out of the arena
        // or if there is a wall or a bomb where we are scanning
        if (BlockY >= ARENA_HEIGHT ||
            m_pArena->m_pArena->IsWall (BlockX, BlockY) ||
            m_pArena->m_pArena->IsBomb (BlockX, BlockY))
        {
            // Stop scanning, there is no enemy near and in front of our bomber in this direction
            break;
        }

        // If there is a bomber where we are scanning
        if (m_pArena->m_pArena->IsBomber (BlockX, BlockY))
        {
            // We have an enemy bomber above that is near
            // and in front of our bomber.
            return true;
        }

        // Continue scanning (go down)
        BlockY++;
    }

    // We scanned in every directions from the bomber.
    // There is no enemy bomber near and in front of our bomber.
    return false;
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

bool CAiBomber::EnemyNear (int BlockX, int BlockY)
{
    // Scan the players
    for (int Index = 0 ; Index < MAX_PLAYERS ; Index++)
    {
        // If the current player is not the one we are controlling
        // and the bomber of this player exists and is alive
        // and the manhattan distance between him and the tested block is not too big
        // and with big probability
        if (Index != m_Player &&
            m_pArena->m_pArena->GetBomber(Index).Exist() && 
            m_pArena->m_pArena->GetBomber(Index).IsAlive() && 
            ABS (m_pArena->m_pArena->GetBomber(Index).GetBlockX() - BlockX) + 
            ABS (m_pArena->m_pArena->GetBomber(Index).GetBlockY() - BlockY) <= 3 &&
            RANDOM(100) < 92)
        {
            // There is an enemy not far from the tested block
            return true;
        }
    }

    // There is no enemy not far from the tested block
    return false;
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

bool CAiBomber::DropBombOK (int BlockX, int BlockY)
{
    int Depth;
    int DangerBlockX;
    int DangerBlockY;

    // parmi les cases sans danger consecutives a partir du bomber, il faut qu'il
    // y en reste au moins 1 sans danger quand on aura pose la bombe
    //*********
    // on determine la distance la plus courte pour aller a une case sans danger une
    // fois qu'on aura pose la bombe. si elle est trop longue (genre >= 4) on pose pas.

    // If the tested block is NOT accessible to our bomber
    if (m_Accessible[BlockX][BlockY] == -1)
    {
        // Why would be drop a bomb here then??
        return false;
    }

    if (m_pArena->m_pArena->GetArenaCloser().GetNumberOfBlocksLeft() <= 15)
    {
        return false;
    }

    if (m_pArena->m_pArena->IsBomb (BlockX, BlockY))
    {
        return false;
    }

    if (m_pArena->m_Danger[BlockX][BlockY] == DANGER_NONE)
    {
        if ((BlockX - 1 < 0             || m_pArena->m_Danger[BlockX-1][BlockY] != DANGER_NONE) &&
            (BlockX + 1 >= ARENA_WIDTH  || m_pArena->m_Danger[BlockX+1][BlockY] != DANGER_NONE) &&
            (BlockY - 1 < 0             || m_pArena->m_Danger[BlockX][BlockY-1] != DANGER_NONE) &&
            (BlockX + 1 >= ARENA_HEIGHT || m_pArena->m_Danger[BlockX][BlockY+1] != DANGER_NONE))
        {
            return false;
        }
    }

    if (m_pBomber->GetSickness() == SICK_COLIC)
    {
        return false;
    }

    // If a bomb is dropped on the tested block then of course one
    // accessible block will be endangered since the tested block
    // is accessible to our bomber.
    int AccessibleEndangered = 1;

    //-------------------------------------------------------------------------------------------
    // Make a fuzzy estimation of the flame size of our bombs (more human than exact flame size)
    //-------------------------------------------------------------------------------------------

    // Get the exact flame size of our bombs
    int FlameSize = m_pArena->m_pArena->GetBomber(m_Player).GetFlameSize();

    // If it exceeds a certain size
    if (FlameSize >= 4)
    {
        // According to the flame size, make the flame size bigger
        switch (FlameSize)
        {
            case 4  : FlameSize =  5; break; // Flame size estimation error is low
            case 5  : FlameSize =  7; break; // Flame size estimation error is medium
            case 6  : FlameSize =  8; break; // Flame size estimation error is medium
            default : FlameSize = 99; break; // Flame size estimation error is high
        }
    }

    // We are now going to simulate an explosion whose center
    // would be on the tested block (function parameters).

    //----------------------------------------------------------------------------------------
    // Simulate the flame ray (right) of the explosion. Count how many accessible blocks 
    // are endangered by this flame ray. Check if it won't hit a valuable item or a wall 
    // that is burning or that will soon burn.
    //----------------------------------------------------------------------------------------

    // Start scanning one block to the right of the tested block
    DangerBlockX = BlockX + 1;
    DangerBlockY = BlockY;

    // First block to scan
    Depth = 0;

    // While there is no obstacle (wall, bomb)
    // and we didn't finish scanning the flame ray of the simulated explosion
    while (!m_pArena->m_pArena->IsBomb (DangerBlockX, DangerBlockY) && 
           !m_pArena->m_pArena->IsWall (DangerBlockX, DangerBlockY) && 
           Depth <= FlameSize)
    {
        // The block we are scanning is accessible and would be endangered by the bomb
        AccessibleEndangered++;

        // If there is an item on the block we are scanning
        // and this item is at least a little bit interesting
        if (m_pArena->m_pArena->IsItem (DangerBlockX, DangerBlockY) && 
            ItemMark (DangerBlockX, DangerBlockY) > 0)
        {
            // We should not drop a bomb here, this item seems to be interesting
            return false;
        }
        
        // Continue scanning (go right)
        DangerBlockX++;
        Depth++;
    }

    // If the scan was stopped because of a wall
    // and this wall is burning or will burn soon
    if (m_pArena->m_pArena->IsWall (DangerBlockX, DangerBlockY) && 
        m_pArena->m_WallBurn[DangerBlockX][DangerBlockY])
    {
        // We should not drop a bomb here, there could be an item under this wall!
        return false;
    }

    //----------------------------------------------------------------------------------------
    // Simulate the flame ray (left) of the explosion. Count how many accessible blocks 
    // are endangered by this flame ray. Check if it won't hit a valuable item or a wall 
    // that is burning or that will soon burn.
    //----------------------------------------------------------------------------------------

    // Start scanning on this block
    DangerBlockX = BlockX - 1;
    DangerBlockY = BlockY;

    // First block to scan
    Depth = 0;

    // While there is no obstacle (wall, bomb)
    // and we didn't finish scanning the flame ray of the simulated explosion
    while (!m_pArena->m_pArena->IsBomb (DangerBlockX, DangerBlockY) && 
           !m_pArena->m_pArena->IsWall (DangerBlockX, DangerBlockY) && 
           Depth <= FlameSize)
    {
        // The block we are scanning is accessible and would be endangered by the bomb
        AccessibleEndangered++;
    
        // If there is an item on the block we are scanning
        // and this item is at least a little bit interesting
        if (m_pArena->m_pArena->IsItem (DangerBlockX, DangerBlockY) && 
            ItemMark (DangerBlockX, DangerBlockY) > 0)
        {
            // We should not drop a bomb here, this item seems to be interesting
            return false;
        }
        
        // Continue scanning (go left)
        DangerBlockX--;
        Depth++;
    }

    // If the scan was stopped because of a wall
    // and this wall is burning or will burn soon
    if (m_pArena->m_pArena->IsWall (DangerBlockX, DangerBlockY) && 
        m_pArena->m_WallBurn[DangerBlockX][DangerBlockY])
    {
        // We should not drop a bomb here, there could be an item under this wall!
        return false;
    }

    //----------------------------------------------------------------------------------------
    // Simulate the flame ray (up) of the explosion. Count how many accessible blocks 
    // are endangered by this flame ray. Check if it won't hit a valuable item or a wall 
    // that is burning or that will soon burn.
    //----------------------------------------------------------------------------------------

    // Start scanning on this block
    DangerBlockX = BlockX;
    DangerBlockY = BlockY - 1;

    // First block to scan
    Depth = 0;

    // While there is no obstacle (wall, bomb)
    // and we didn't finish scanning the flame ray of the simulated explosion
    while (!m_pArena->m_pArena->IsBomb (DangerBlockX, DangerBlockY) && 
           !m_pArena->m_pArena->IsWall (DangerBlockX, DangerBlockY) && 
           Depth <= FlameSize)
    {
        // The block we are scanning is accessible and would be endangered by the bomb
        AccessibleEndangered++;

        // If there is an item on the block we are scanning
        // and this item is at least a little bit interesting
        if (m_pArena->m_pArena->IsItem (DangerBlockX, DangerBlockY) && 
            ItemMark (DangerBlockX, DangerBlockY) > 0)
        {
            // We should not drop a bomb here, this item seems to be interesting
            return false;
        }
        
        // Continue scanning (go up)
        DangerBlockY--;
        Depth++;
    }

    // If the scan was stopped because of a wall
    // and this wall is burning or will burn soon
    if (m_pArena->m_pArena->IsWall (DangerBlockX, DangerBlockY) && 
        m_pArena->m_WallBurn[DangerBlockX][DangerBlockY])
    {
        // We should not drop a bomb here, there could be an item under this wall!
        return false;
    }

    //----------------------------------------------------------------------------------------
    // Simulate the flame ray (down) of the explosion. Count how many accessible blocks 
    // are endangered by this flame ray. Check if it won't hit a valuable item or a wall 
    // that is burning or that will soon burn.
    //----------------------------------------------------------------------------------------

    // Start scanning on this block
    DangerBlockX = BlockX;
    DangerBlockY = BlockY + 1;

    // First block to scan
    Depth = 0;

    // While there is no obstacle (wall, bomb)
    // and we didn't finish scanning the flame ray of the simulated explosion
    while (!m_pArena->m_pArena->IsBomb (DangerBlockX, DangerBlockY) && 
           !m_pArena->m_pArena->IsWall (DangerBlockX, DangerBlockY) && 
           Depth <= FlameSize)
    {
        // The block we are scanning is accessible and would be endangered by the bomb
        AccessibleEndangered++;
    
        // If there is an item on the block we are scanning
        // and this item is at least a little bit interesting
        if (m_pArena->m_pArena->IsItem (DangerBlockX, DangerBlockY) && 
            ItemMark (DangerBlockX, DangerBlockY) > 0)
        {
            // We should not drop a bomb here, this item seems to be interesting
            return false;
        }
        
        // Continue scanning (go down)
        DangerBlockY++;
        Depth++;
    }

    // If the scan was stopped because of a wall
    // and this wall is burning or will burn soon
    if (m_pArena->m_pArena->IsWall (DangerBlockX, DangerBlockY) && 
        m_pArena->m_WallBurn[DangerBlockX][DangerBlockY])
    {
        // We should not drop a bomb here, there could be an item under this wall!
        return false;
    }

    return (m_NumAccessible > AccessibleEndangered);
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

int CAiBomber::ItemMark (int BlockX, int BlockY)
{
    // If there is no item on the tested block
    // or if this is a skull or burning item
    // or if the item is not accessible to our bomber
    // or if the item will die in less than one second
    // or if the item is in danger and our bomber is too far away from this item
    if (!m_pArena->m_pArena->IsItem (BlockX, BlockY) ||
        m_pArena->m_pArena->IsSkullItem (BlockX, BlockY) ||
        m_pArena->m_pArena->IsBurningItem (BlockX, BlockY) ||
        m_Accessible[BlockX][BlockY] == -1 ||
        m_pArena->m_DangerTimeLeft[BlockX][BlockY] < 1.0f ||
        (m_pArena->m_Danger[BlockX][BlockY] != DANGER_NONE && m_Accessible[BlockX][BlockY] >= 3))
    {
        // Worst mark, this item is not interesting
        return 0;
    }

    // Initialize item mark to return to zero.
    int Mark = 0;

    //----------------------------------------
    // Take the type of the item into account
    //----------------------------------------

    // Variable where to retrieve the type of the item on the tested block
    EItemType ItemType = ITEM_NONE;

    // Scan the items of the arena
    for (int Index = 0 ; Index < m_pArena->m_pArena->MaxItems() ; Index++)
    {
        // If the current item is on the tested block
        if (m_pArena->m_pArena->GetItem(Index).GetBlockX() == BlockX &&
            m_pArena->m_pArena->GetItem(Index).GetBlockY() == BlockY)
        {
            // Get the type of this item and get out
            ItemType = m_pArena->m_pArena->GetItem(Index).GetType();
            break;
        }
    }

    ASSERT (ItemType != ITEM_NONE);

    // According to the item type, modify the current item mark
    switch (ItemType)
    {
        case ITEM_BOMB   : Mark += 10; break;
        case ITEM_FLAME  : Mark += 10; break;
        case ITEM_KICK   : Mark += 30; break;
        case ITEM_ROLLER : Mark += 20; break;
        case ITEM_THROW  : Mark += 50; break;
        case ITEM_PUNCH  : Mark += 60; break;
        default: break;
    }

    //--------------------------------------------------------------
    // Take other details of this item into account (distance, etc)
    //--------------------------------------------------------------

    // If the item is near our bomber
    if (m_Accessible[BlockX][BlockY] <= 3)
    {
        // This item is much more interesting, increase the mark
        Mark += 5;
    }
    // If the item is not very far away from our bomber
    else if (m_Accessible[BlockX][BlockY] <= 6)
    {
        // This item is quite interesting, increase the mark
        Mark += 3;
    }

    // If the item is not in a dead end
    if (m_pArena->m_DeadEnd[BlockX][BlockY] == -1)
    {
        // Increase the item mark
        Mark += 10;
    }

    // OK, we estimated how good it would be to pick up this item.
    // Return the item mark.
    return Mark;
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

/*

ALGO :

si on est en danger ou l'arene est presque completement fermee
    mode defense

si ca semble ok d'attaquer
    mode attaque

chercher un item

si on a trouve un item
    mode item

si je suis malade et que je peux aller toucher un bomber ennemi
    pathfinding goto pour l'atteindre

chercher des murs qui sont entrain de bruler

si on en trouve
    mode item pour s'y rendre

chercher le bloc avec le plus de murs cassables adjacents
    mode item pour s'y rendre et y poser une bombe

si on a pas pu trouver quelque chose d'interessant a faire...
    mode balade

*/

void CAiBomber::ModeThink (void)
{
    int BlockX;
    int BlockY;
    bool FoundItem;
    int BestGoalBlockX = -1;
    int BestGoalBlockY = -1;
    int BestMark;
    bool FoundSoftWallBurn;
    int BestDistance;

    //--------------------------------------------------
    // Check if we should defend.
    // Defend when you are on a dangerous block or when
    // the arena is almost entirely closed.
    //--------------------------------------------------

    // If the block where the bomber is is dangerous
    // or if there are not many opened blocks left in the arena
    if (m_pArena->m_Danger[m_BlockHereX][m_BlockHereY] != DANGER_NONE ||
        m_pArena->m_pArena->GetArenaCloser().GetNumberOfBlocksLeft() <= 10)
    {
        // Switch to the defence mode so that the bomber goes to a safer block
        SetComputerMode (COMPUTERMODE_DEFENCE);

        // OK, get out since we decided what to do
        return;
    }

    //---------------------------------------------------------
    // Check if we should to attack.
    // Attack when there is an enemy near and in front of you.
    //---------------------------------------------------------

    // If there is an enemy near and in front of our bomber
    // and it is ok to drop a bomb where our bomber is
    // with quite big probability
    if (EnemyNearAndFront() &&
        DropBombOK(m_BlockHereX,m_BlockHereY) &&
        RANDOM(100) < 70)
    {
        // Switch to the attack mode to drop a bomb
        SetComputerMode (COMPUTERMODE_ATTACK);

        // OK, get out since we decided what to do
        return;
    }

    //----------------------------------------------
    // Check if there is a cool item to pick up
    // Make an estimation of the mark of each item.
    //----------------------------------------------

    // Assume no item is found
    FoundItem = false;
    
    // Current best item mark is the worst mark
    BestMark = 0;

    // Scan the blocks of the AI view
    for (BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
    {
        for (BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
        {
            // If the block is outside the arena
            if (BlockX < 0 || BlockX > ARENA_WIDTH - 1 ||
                BlockY < 0 || BlockY > ARENA_HEIGHT - 1)
            {
                // Next block of the AI view
                continue;
            }
            // If the block is inside the arena
            else
            {
                // Get the item mark of this block
                int Mark = ItemMark (BlockX, BlockY);
                
                // If this item is at least a little bit interesting
                // and its mark is higher than the current best mark
                // (if its mark is equal to the best mark, half
                // probability to be the best mark)
                if (Mark > 0 && (Mark > BestMark || (Mark == BestMark && RANDOM(100) >= 50)))
                {
                    // We found an item to pick up
                    FoundItem = true;

                    // Save the item's block coordinates and the item mark
                    BestGoalBlockX = BlockX;
                    BestGoalBlockY = BlockY;
                    BestMark = Mark;
                }
            }
        }
    }

    //----------------------------------------
    // If there is a cool item, go pick it up
    //----------------------------------------

    // If we found a cool item to pick up
    if (FoundItem)
    {
        ASSERT (BestGoalBlockX != -1);
        ASSERT (BestGoalBlockY != -1);

        // Prepare coordinates of the block to go to in the item mode
        m_ItemGoalBlockX = BestGoalBlockX;
        m_ItemGoalBlockY = BestGoalBlockY;
        
        // Don't drop a bomb when you reach the goal,
        // because you just have to go pick up the item there.
        m_ItemDropBomb = false;

        // Switch to item mode to execute what was decided
        SetComputerMode (COMPUTERMODE_ITEM);

        // OK, get out since we decided what to do
        return;
    }

    //*********
    // SICKNESS
    //*********

    if (m_pBomber->GetSickness() != SICK_NOTSICK)
    {
        // Scan the players
        for (int Index = 0 ; Index < MAX_PLAYERS ; Index++)
        {
            if (Index != m_Player &&
                m_pArena->m_pArena->GetBomber(Index).Exist() && 
                m_pArena->m_pArena->GetBomber(Index).IsAlive())                
            {
                BlockX = m_pArena->m_pArena->GetBomber(Index).GetBlockX();
                BlockY = m_pArena->m_pArena->GetBomber(Index).GetBlockY();

                if (m_Accessible[BlockX][BlockY] != -1)
                {
                    GoTo (BlockX, BlockY);

                    return;
                }
            }
        }
    }

    //------------------------------------------------------------------------
    // Check if there are soft walls that will burn soon or that are burning.
    // It is interesting to take a look at these walls because items could
    // be under them, which is a very quick way to get items. Waiting near
    // these walls is a quite good idea.
    //------------------------------------------------------------------------

    // Assume a burning soft wall or a soft wall that will soon burn is not found
    FoundSoftWallBurn = false;
    
    // Coordinates of the best block to go to if such a wall was found
    BestGoalBlockX = m_BlockHereX;
    BestGoalBlockY = m_BlockHereY;

    // Current shortest distance to such a wall is the maximum
    BestDistance = 999;

    // Scan the blocks of the AI view
    for (BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
    {
        for (BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
        {
            // If the block is outside the arena
            if (BlockX < 0 || BlockX > ARENA_WIDTH - 1 ||
                BlockY < 0 || BlockY > ARENA_HEIGHT - 1)
            {
                // Next block of the AI view
                continue;
            }
            // If the block is inside the arena
            else
            {
                if (m_PseudoAccessible[BlockX][BlockY] != -1 &&
                    m_PseudoAccessible[BlockX][BlockY] <= 5 &&
                    (
                        BestDistance > m_PseudoAccessible[BlockX][BlockY] 
                        ||
                        (
                            BestDistance == m_PseudoAccessible[BlockX][BlockY] 
                            &&
                            RANDOM(100) >= 50
                        )
                    )// &&
                    //(m_pArena->m_DeadEnd[BlockX][BlockY] == -1 || !EnemyNear(BlockX,BlockY))
                    &&
                    (m_pArena->m_WallBurn[BlockX-1][BlockY] ||
                     m_pArena->m_WallBurn[BlockX+1][BlockY] ||
                     m_pArena->m_WallBurn[BlockX][BlockY-1] ||
                     m_pArena->m_WallBurn[BlockX][BlockY+1]))
                {
                    FoundSoftWallBurn = true;
                    BestGoalBlockX = BlockX;
                    BestGoalBlockY = BlockY;
                    BestDistance = m_PseudoAccessible[BlockX][BlockY];
                }
            }
        }
    }

    //------------------------------------------------
    // If there is a soft wall that will soon burn or
    // that is burning, go near one of these walls.
    //------------------------------------------------

    // If we found a good block to go to
    if (FoundSoftWallBurn)
    {
        if (m_Accessible[BestGoalBlockX][BestGoalBlockY] != -1 &&
            (m_BlockHereX != BestGoalBlockX || 
             m_BlockHereY != BestGoalBlockY))
        {        
            // Prepare coordinates of the block to go to in the item mode
            m_ItemGoalBlockX = BestGoalBlockX;
            m_ItemGoalBlockY = BestGoalBlockY;
        
            // Don't drop a bomb when you reach the goal,
            // because you just have to go pick up the item there.
            m_ItemDropBomb = false;

            // Switch to item mode to execute what was decided
            SetComputerMode (COMPUTERMODE_ITEM);
        }
                
        // OK, get out since we decided what to do
        return;
    }

    //----------------------------------------------------------
    // Find the block close to the highest number of soft walls
    //----------------------------------------------------------

    BestMark = 0;

    // Coordinates of the best block to go to if such a wall was found
    BestGoalBlockX = m_BlockHereX;
    BestGoalBlockY = m_BlockHereY;

    // Scan the blocks of the AI view
    for (BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
    {
        for (BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
        {
            // If the block is outside the arena
            if (BlockX < 0 || BlockX > ARENA_WIDTH - 1 ||
                BlockY < 0 || BlockY > ARENA_HEIGHT - 1)
            {
                // Next block of the AI view
                continue;
            }
            // If the block is inside the arena
            else
            {
                if (m_pArena->m_SoftWallNear[BlockX][BlockY] != -1 && 
                    m_pArena->m_SoftWallNear[BlockX][BlockY] > 0 && 
                    m_Accessible[BlockX][BlockY] != -1 &&
                    m_Accessible[BlockX][BlockY] <= 5 &&
                    (m_pArena->m_DeadEnd[BlockX][BlockY] == -1 || !EnemyNear(BlockX,BlockY)) &&
                    m_pArena->m_Danger[BlockX][BlockY] == DANGER_NONE
                    &&
                    (
                        BestMark < m_BurnMark[m_pArena->m_SoftWallNear[BlockX][BlockY]][m_Accessible[BlockX][BlockY]]
                        ||
                        (
                            BestMark == m_BurnMark[m_pArena->m_SoftWallNear[BlockX][BlockY]][m_Accessible[BlockX][BlockY]]
                            &&
                            RANDOM(100) >= 50
                        )
                    ) &&
                    DropBombOK (BlockX,BlockY))
                {
                    // Save the coordinates of the best block
                    BestGoalBlockX = BlockX;
                    BestGoalBlockY = BlockY;

                    BestMark = m_BurnMark[m_pArena->m_SoftWallNear[BlockX][BlockY]][m_Accessible[BlockX][BlockY]];
                }
            }
        }
    }

    // If we found a good block to go to
    if (BestMark > 0)
    {
        // Prepare coordinates of the block to go to in the item mode
        m_ItemGoalBlockX = BestGoalBlockX;
        m_ItemGoalBlockY = BestGoalBlockY;

        // Drop a bomb when you reach the goal,
        // because you have to burn some walls.
        m_ItemDropBomb = true;
        
        // Switch to the item mode to execute what was decided
        SetComputerMode (COMPUTERMODE_ITEM);

        // OK, get out since we decided what to do
        return;
    }

    // Nothing better to do than walking in random directions.
    // Switch to walk mode.
    SetComputerMode (COMPUTERMODE_WALK);

    m_WalkTime = 0.0f;
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

/*

ALGO :

si il y a un ennemi devant moi et que je peux poser une bombe sans danger
    mode reflechir

si mon but est d'aller poser une bombe pour bruler du mur, et que je ne peux pas poser une bombe sans danger
    mode reflechir

si mon but est de ramasser un item a tel bloc
    actualiser le bloc de but, ou se trouve l'item le plus interessant

se diriger vers le bloc de but en cours

si le bloc de but n'est pas accessible
    mode reflechir

si j'ai atteint le bloc de but
    si je devais y poser une bombe
        poser une bombe
    sinon
        fini, j'ai pris l'item, mode reflechir

*/

void CAiBomber::ModeItem (float DeltaTime)
{
    // Reset the commands to send to the bomber
    m_BomberMove = BOMBERMOVE_NONE;
    m_BomberAction = BOMBERACTION_NONE;
    
    
    if ((
            EnemyNearAndFront() && 
            DropBombOK (m_BlockHereX, m_BlockHereY) && 
            RANDOM(100) < 70
        )
        ||
        (
            m_ItemDropBomb &&
            !DropBombOK (m_ItemGoalBlockX, m_ItemGoalBlockY)
        ))
    {
        // Decide what to do
        SetComputerMode (COMPUTERMODE_THINK);

        // Get out, mode is over
        return;
    }

    // If we are trying to pick up an item
    if (m_pArena->m_pArena->IsItem (m_ItemGoalBlockX, m_ItemGoalBlockY))
    {
        bool FoundItem = false;
        int BestMark = 0;

        // Scan the blocks of the AI view
        for (int BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
        {
            for (int BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
            {
                // If the block is outside the arena
                if (BlockX < 0 || BlockX > ARENA_WIDTH - 1 ||
                    BlockY < 0 || BlockY > ARENA_HEIGHT - 1)
                {
                    // Next block of the AI view
                    continue;
                }
                // If the block is inside the arena
                else
                {
                    // Get the item mark of this block
                    int Mark = ItemMark (BlockX, BlockY);
                
                    // If this item is at least a little bit interesting
                    // and its mark is higher than the current best mark
                    // (if its mark is equal to the best mark, half
                    // probability to be the best mark)
                    if (Mark > 0 && Mark > BestMark)
                    {
                        FoundItem = true;
                        m_ItemGoalBlockX = BlockX;
                        m_ItemGoalBlockY = BlockY;
                        BestMark = Mark;
                    }
                }
            }
        }

        if (FoundItem && BestMark == 0)
        {
            // Decide what to do
            SetComputerMode (COMPUTERMODE_THINK);

            // Get out, mode is over
            return;
        }
    }
    
    // Assume the goal has not been reached yet
    bool GoalReach = false;

    // If the block to go to is accessible to the bomber
    if (m_Accessible[m_ItemGoalBlockX][m_ItemGoalBlockY] != -1)
    {
        // Set the bomber move command so that the bomber goes to the block to go to
        // and return if the goal has been reached.
        GoalReach = GoTo (m_ItemGoalBlockX, m_ItemGoalBlockY);
    }
    // If the block to go to is not accessible to the bomber
    else
    {
        // There is a problem : we're trying to go to a block that is not accessible.
        // Switch to think mode so as to decide what to do.
        SetComputerMode (COMPUTERMODE_THINK);

        // Get out, no need to stay here
        return;
    }

    // If the goal was reached and the bomber has to drop a bomb and the bomber is Able to drop a bomb now
    if (GoalReach && m_ItemDropBomb && m_pBomber->GetTotalBombs() - m_pBomber->GetUsedBombsCount() > 0)
    {
        // Set the bomber action to drop a bomb
        m_BomberAction = BOMBERACTION_ACTION1;

        // The goal is the block where the bomber is (so in fact no need to move)
        m_ItemGoalBlockX = m_BlockHereX;
        m_ItemGoalBlockY = m_BlockHereY;

        // The bomb will now be dropped, no need to drop another bomb now
        m_ItemDropBomb = false;
    }
    // If the goal was reached and the bomber does not have to drop a bomb
    else if (GoalReach && !m_ItemDropBomb)
    {
        // What was decided in the think mode has been entirely executed.
        // Switch to think mode to decide what to do now.
        SetComputerMode (COMPUTERMODE_THINK);
    }
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

/*

ALGO :

Poser une bombe
Passer en mode reflechir

*/

void CAiBomber::ModeAttack (void)
{
    // Reset the commands to send to the bomber
    m_BomberMove = BOMBERMOVE_NONE;
    m_BomberAction = BOMBERACTION_ACTION1;

    // What was decided in the think mode has been entirely executed.
    // Switch to think mode to decide what to do now.
    SetComputerMode (COMPUTERMODE_THINK);
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

/* 

ALGO :

si le bomber n'est pas en danger
    mode reflechir
sinon
    chercher une case non dangereuse (qu'on peut atteindre sans avoir d'items speciaux)
    si aucune case trouvee
        chercher une case non dangereuse qu'on peut atteindre avec le kick item
        si case trouvee
            enregistrer le mouvement (simple mouvement horizontal ou vertical, pas besoin de goto)
        sinon
            chercher la case accessible la moins dangereuse possible
    enregistrer le mouvement (pathfinding avec goto)

*/

void CAiBomber::ModeDefence (float DeltaTime)
{
    m_BomberAction = BOMBERACTION_NONE;
    
    // If the bomber is not in danger
    if (m_pArena->m_Danger[m_BlockHereX][m_BlockHereY] == DANGER_NONE && m_pArena->m_pArena->GetArenaCloser().GetNumberOfBlocksLeft() > 10)
    {
        // Reset commands to send to the bomber in order 
        // to stop moving when the bomber is in a safe block.
        m_BomberMove = BOMBERMOVE_NONE;
        m_BomberAction = BOMBERACTION_NONE;
        
        // No need to defend, switch to think mode so as to decide what to do.
        SetComputerMode (COMPUTERMODE_THINK);

        // Get out, this mode is over
        return;
    }

    // Assume we didn't find any good block to go to
    bool Found = false;

    // Coordinates of the best block to go to
    int BestBlockX = -1;
    int BestBlockY = -1;

    int BlockX;
    int BlockY;
    
    // next blocks
    int NextBlockX = -1;
    int NextBlockY = -1;

    // Distance of the best block to go to
    int BestDistance = 999;

    bool DeadEnd = true;
    bool twoBombs = false; // are there two bombs beside us?
    bool breaking = false; // first break executed.
    
    // a timestamp for the log
    /*time_t long_time;
    struct tm *newtime;

    time( &long_time );
    newtime = localtime( &long_time );*/
    
    // Scan the blocks of the AI view
    for (BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
    {
        for (BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
        {
            // If the block is outside the arena
            if (BlockX < 0 || BlockX > ARENA_WIDTH - 1 ||
                BlockY < 0 || BlockY > ARENA_HEIGHT - 1)
            {
                // Next block in the AI view
                continue;
            }
            // If the block is inside the arena
            else
            {
                // If this block is accessible
                // and this block is not in danger
                // and this block is closer than the closest good block we saved
                if (m_Accessible[BlockX][BlockY] != -1 &&
                    (EnemyNear(BlockX,BlockY) ? m_pArena->m_DeadEnd[BlockX][BlockY] == -1 : (m_pArena->m_DeadEnd[BlockX][BlockY] != -1 || !DeadEnd)) &&
                    m_pArena->m_Danger[BlockX][BlockY] == DANGER_NONE &&
                    !m_pArena->m_pArena->IsSkullItem (BlockX, BlockY)
                    &&
                    (
                        m_Accessible[BlockX][BlockY] < BestDistance 
                        ||
                        (
                            m_Accessible[BlockX][BlockY] == BestDistance 
                            &&
                            RANDOM(100) >= 50
                        )
                    ))
                {
                    // We found a good block to go to
                    Found = true;

                    // Save the coordinates and the distance of this block
                    BestBlockX = BlockX;
                    BestBlockY = BlockY;
                    BestDistance = m_Accessible[BlockX][BlockY];
                    DeadEnd = (m_pArena->m_DeadEnd[BlockX][BlockY] != -1);

                    breaking = true;
                    break;
                }
            }
        }
        
        if (breaking) break;
    }

    float BestDangerTimeLeft;

    if (!Found)
    {
        if (m_pBomber->CanKickBombs () || m_pBomber->CanPunchBombs ())
        {
            BestDangerTimeLeft = 0.0f;
            
            // Scan the blocks of the AI view
            for (BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
            {
                for (BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
                {
                    // If the block is outside the arena
                    if (BlockX < 0 || BlockX > ARENA_WIDTH - 1 ||
                        BlockY < 0 || BlockY > ARENA_HEIGHT - 1)
                    {
                        // Next block in the AI view
                        continue;
                    }
                    // If the block is inside the arena
                    else
                    {
                        if (m_PseudoAccessible[BlockX][BlockY] != -1 &&
                            m_PseudoAccessible[BlockX][BlockY] <= 4 &&
                            m_pArena->m_pArena->IsBomb (BlockX, BlockY) &&
                            (BlockX == m_BlockHereX || BlockY == m_BlockHereY) &&
                            (BlockX != m_BlockHereX || BlockY != m_BlockHereY) &&
                            m_pArena->m_DangerTimeLeft[BlockX][BlockY] >BestDangerTimeLeft)
                        {
                            // We found a good block to go to
                            Found = true;

                            // Save the coordinates and the distance of this block
                            BestBlockX = BlockX;
                            BestBlockY = BlockY;
                            BestDangerTimeLeft = m_pArena->m_DangerTimeLeft[BlockX][BlockY];
                        }
                    }
                }
            }
        }

        if (Found)
        {
            if (m_BlockHereX - BestBlockX > 0)
            {
                m_BomberMove = BOMBERMOVE_LEFT;
                
                // find out next block
                if (m_BlockHereX > 0)
                {
                    NextBlockX = m_BlockHereX - 1;
                    NextBlockY = m_BlockHereY;
									
                    if (m_BlockHereX > 1)
                    {
                        // check if there is more than one bomb besides us.
                        if (m_pArena->m_pArena->IsBomb (NextBlockX, NextBlockY) &&
                            m_pArena->m_pArena->IsBomb (NextBlockX - 1, NextBlockY))
                        {
                            twoBombs = true;
                        }
                    }
                }
            }
            else if (m_BlockHereX - BestBlockX < 0)
            {
                m_BomberMove = BOMBERMOVE_RIGHT;

                // find out next block
                if (m_BlockHereX < ARENA_WIDTH - 1)
                {
                    NextBlockX = m_BlockHereX + 1;
                    NextBlockY = m_BlockHereY;

                    if (m_BlockHereX < ARENA_WIDTH - 2)
                    {
                        // check if there is more than one bomb besides us.
                        if (m_pArena->m_pArena->IsBomb (NextBlockX, NextBlockY) &&
                            m_pArena->m_pArena->IsBomb (NextBlockX + 1, NextBlockY))
                        {
                            twoBombs = true;
                        }
                    }
                }
            }
            else if (m_BlockHereY - BestBlockY > 0)
            {
                m_BomberMove = BOMBERMOVE_UP;

                // find out next block
                if (m_BlockHereY > 0)
                {
                    NextBlockX = m_BlockHereX;
                    NextBlockY = m_BlockHereY - 1;

                    if (m_BlockHereY > 1)
                    {
	                    // check if there is more than one bomb besides us.
                        if (m_pArena->m_pArena->IsBomb (NextBlockX, NextBlockY) &&
                            m_pArena->m_pArena->IsBomb (NextBlockX, NextBlockY - 1))
                        {
    	                    twoBombs = true;
	                    }
                    }
                }
            }
            else if (m_BlockHereY - BestBlockY < 0)
            {
                m_BomberMove = BOMBERMOVE_DOWN;

                // find out next block
                if (m_BlockHereY < ARENA_HEIGHT - 1)
                {
                    NextBlockX = m_BlockHereX;
                    NextBlockY = m_BlockHereY + 1;
									
                    if (m_BlockHereY < ARENA_HEIGHT - 2)
                    {
                        // check if there is more than one bomb besides us.
                        if (m_pArena->m_pArena->IsBomb (NextBlockX, NextBlockY) &&
                            m_pArena->m_pArena->IsBomb (NextBlockX, NextBlockY + 1))
                        {
                            twoBombs = true;
                        }
                    }
				}
            }
            else
            {
                m_BomberMove = BOMBERMOVE_NONE;
            }
 
            // If the bomber move is a real move
            if (m_BomberMove != BOMBERMOVE_NONE)
            {
                // if we want to punch a bomb away, this is the time!
                // punch 'em, if there are two bombs in a row besides us or
                // we don't have the ability to kick bombs!
			    if (NextBlockX != -1 && NextBlockY != -1)
                {
                    if ((!m_pBomber->CanKickBombs () || twoBombs) &&
                        m_pBomber->CanPunchBombs () &&
                        m_pArena->m_pArena->IsBomb (NextBlockX, NextBlockY))
                    {
                        m_BomberAction = BOMBERACTION_ACTION2;
                    }
                }

                // Send this bomber move to the bomber as many seconds as the time
                // a bomber takes to walk through an entire block.
                m_BomberMoveTimeLeft = BLOCK_SIZE * 1.0f / m_pBomber->GetPixelsPerSecond();
            }
            // If the bomber move is not a real move
            else
            {
                // Send this bomber move only on this update
                m_BomberMoveTimeLeft = 0.0f;
            }

            return;
        }
    }

    if (!Found)
    {
        //------------------------------------
        // Determine the less dangerous block
        //------------------------------------
        
        BestDangerTimeLeft = 0.0f;

        // Scan the blocks of the AI view
        for (BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
        {
            for (BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
            {
                // If the block is outside the arena
                if (BlockX < 0 || BlockX > ARENA_WIDTH - 1 ||
                    BlockY < 0 || BlockY > ARENA_HEIGHT - 1)
                {
                    // Next block in the AI view
                    continue;
                }
                // If the block is inside the arena
                else
                {
                    // If this block is accessible
                    // and this block is not in danger
                    // and this block is closer than the closest good block we saved
                    if (m_Accessible[BlockX][BlockY] != -1 &&
                        m_pArena->m_DangerTimeLeft[BlockX][BlockY] > BestDangerTimeLeft)
                    {
                        // We found a good block to go to
                        Found = true;

                        // Save the coordinates and the distance of this block
                        BestBlockX = BlockX;
                        BestBlockY = BlockY;
                        BestDistance = m_Accessible[BlockX][BlockY];
                    
                        BestDangerTimeLeft = m_pArena->m_DangerTimeLeft[BlockX][BlockY];
                    }
                }
            }
        }
    }
    
    // If a good block to go to was found
    if (Found)
    {
        ASSERT (BestBlockX != -1);
        ASSERT (BestBlockY != -1);
        
        // Set the bomber move to send to the bomber so that the bomber goes to the best block
        GoTo (BestBlockX, BestBlockY);
    }
    // If a good block to go to was not found
    else
    {
        // Move nowhere
        m_BomberMove = BOMBERMOVE_NONE;
        m_BomberMoveTimeLeft = 0.0f;
    }
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

/*

ALGO :

si on est en danger
    mode reflechir

si on a un ennemi devant soi et qu'on pourrait poser une bombe
    mode reflechir

si ca fait une seconde qu'on se balade
    mode reflechir

donner une note d'interet a chaque quart de l'arene (le centre etant notre position)
choisir un mouvement selon l'interet, le danger, notre precedent mouvement, etc
enregistrer le mouvement

*/

void CAiBomber::ModeWalk (float DeltaTime)
{
    // Set no bomber action to send to the bomber (we are just walking!)
    m_BomberAction = BOMBERACTION_NONE;
    
    // If the bomber is in danger
    if (m_pArena->m_Danger[m_BlockHereX][m_BlockHereY] != DANGER_NONE)
    {
        // While walking we got into trouble! Decide what to do
        SetComputerMode (COMPUTERMODE_THINK);

        // Get out, mode is over
        return;
    }

    if (EnemyNearAndFront() && DropBombOK(m_BlockHereX,m_BlockHereY))
    {
        // Decide what to do
        SetComputerMode (COMPUTERMODE_THINK);

        // Get out, mode is over
        return;
    }

    int BlockX;
    int BlockY;

    if (m_WalkTime >= 1.0f)
    {
        SetComputerMode (COMPUTERMODE_THINK);

        return;
    }

    int MarkDownRight = 0;
    int MarkDownLeft = 0;
    int MarkUpLeft = 0;
    int MarkUpRight = 0;
    
    for (BlockX = m_BlockHereX ; BlockX < ARENA_WIDTH ; BlockX++)
    {
        for (BlockY = m_BlockHereY ; BlockY < ARENA_HEIGHT ; BlockY++)
        {
            if (BlockX == m_BlockHereX && BlockY == m_BlockHereY)
            {
                continue;
            }
            
            if (m_pArena->m_pArena->IsSoftWall (BlockX, BlockY))
            {
                MarkDownRight += 2;
            }
            else if (m_pArena->m_pArena->IsItem (BlockX, BlockY) && !m_pArena->m_pArena->IsSkullItem (BlockX, BlockY))
            {
                MarkDownRight += 10;
            }
            else if (m_pArena->m_pArena->IsBomber (BlockX, BlockY))
            {
                MarkDownRight += 5;
            }
        }
    }

    for (BlockX = m_BlockHereX ; BlockX >= 0 ; BlockX--)
    {
        for (BlockY = m_BlockHereY ; BlockY < ARENA_HEIGHT ; BlockY++)
        {
            if (BlockX == m_BlockHereX && BlockY == m_BlockHereY)
            {
                continue;
            }
            
            if (m_pArena->m_pArena->IsSoftWall (BlockX, BlockY))
            {
                MarkDownLeft += 2;
            }
            else if (m_pArena->m_pArena->IsItem (BlockX, BlockY) && !m_pArena->m_pArena->IsSkullItem (BlockX, BlockY))
            {
                MarkDownLeft += 10;
            }
            else if (m_pArena->m_pArena->IsBomber (BlockX, BlockY))
            {
                MarkDownLeft += 5;
            }
        }
    }

    for (BlockX = m_BlockHereX ; BlockX >= 0 ; BlockX--)
    {
        for (BlockY = m_BlockHereY ; BlockY >= 0 ; BlockY--)
        {
            if (BlockX == m_BlockHereX && BlockY == m_BlockHereY)
            {
                continue;
            }
            
            if (m_pArena->m_pArena->IsSoftWall (BlockX, BlockY))
            {
                MarkUpLeft += 2;
            }
            else if (m_pArena->m_pArena->IsItem (BlockX, BlockY) && !m_pArena->m_pArena->IsSkullItem (BlockX, BlockY))
            {
                MarkUpLeft += 10;
            }
            else if (m_pArena->m_pArena->IsBomber (BlockX, BlockY))
            {
                MarkUpLeft += 5;
            }
        }
    }

    for (BlockX = m_BlockHereX ; BlockX < ARENA_WIDTH ; BlockX++)
    {
        for (BlockY = m_BlockHereY ; BlockY >= 0 ; BlockY--)
        {
            if (BlockX == m_BlockHereX && BlockY == m_BlockHereY)
            {
                continue;
            }
            
            if (m_pArena->m_pArena->IsSoftWall (BlockX, BlockY))
            {
                MarkUpRight += 2;
            }
            else if (m_pArena->m_pArena->IsItem (BlockX, BlockY) && !m_pArena->m_pArena->IsSkullItem (BlockX, BlockY))
            {
                MarkUpRight += 10;
            }
            else if (m_pArena->m_pArena->IsBomber (BlockX, BlockY))
            {
                MarkUpRight += 5;
            }
        }
    }

    if (!MarkDownRight && !MarkDownLeft && !MarkUpLeft && !MarkUpRight)
    {
        return;
    }

    // Is a move in a single direction possible?
    bool CanMoveUp    = !m_pArena->m_pArena->IsWall(m_BlockUpX, m_BlockUpY) && !m_pArena->m_pArena->IsBomb(m_BlockUpX, m_BlockUpY);
    bool CanMoveDown  = !m_pArena->m_pArena->IsWall(m_BlockDownX, m_BlockDownY) && !m_pArena->m_pArena->IsBomb(m_BlockDownX, m_BlockDownY);
    bool CanMoveLeft  = !m_pArena->m_pArena->IsWall(m_BlockLeftX, m_BlockLeftY) && !m_pArena->m_pArena->IsBomb(m_BlockLeftX, m_BlockLeftY);
    bool CanMoveRight = !m_pArena->m_pArena->IsWall(m_BlockRightX, m_BlockRightY) && !m_pArena->m_pArena->IsBomb(m_BlockRightX, m_BlockRightY);

    EDanger DangerUp    = m_pArena->m_Danger[m_BlockUpX   ][m_BlockUpY];
    EDanger DangerDown  = m_pArena->m_Danger[m_BlockDownX ][m_BlockDownY];
    EDanger DangerLeft  = m_pArena->m_Danger[m_BlockLeftX ][m_BlockLeftY];
    EDanger DangerRight = m_pArena->m_Danger[m_BlockRightX][m_BlockRightY];

    if (MarkDownRight >= MarkDownLeft &&
        MarkDownRight >= MarkUpLeft &&
        MarkDownRight >= MarkUpRight)
    {
        if (RANDOM(100) >= 50)
        {
                 if (DangerDown  == DANGER_NONE && m_BomberMove != BOMBERMOVE_UP    && CanMoveDown ) m_BomberMove = BOMBERMOVE_DOWN;
            else if (DangerRight == DANGER_NONE && m_BomberMove != BOMBERMOVE_LEFT  && CanMoveRight) m_BomberMove = BOMBERMOVE_RIGHT;
            else if (DangerUp    == DANGER_NONE && m_BomberMove != BOMBERMOVE_DOWN  && CanMoveUp   ) m_BomberMove = BOMBERMOVE_UP;
            else if (DangerLeft  == DANGER_NONE && m_BomberMove != BOMBERMOVE_RIGHT && CanMoveLeft ) m_BomberMove = BOMBERMOVE_LEFT;
            else                                                                                         m_BomberMove = BOMBERMOVE_NONE;
        }
        else
        {
                 if (DangerRight == DANGER_NONE && m_BomberMove != BOMBERMOVE_LEFT  && CanMoveRight) m_BomberMove = BOMBERMOVE_RIGHT;
            else if (DangerDown  == DANGER_NONE && m_BomberMove != BOMBERMOVE_UP    && CanMoveDown ) m_BomberMove = BOMBERMOVE_DOWN;
            else if (DangerLeft  == DANGER_NONE && m_BomberMove != BOMBERMOVE_RIGHT && CanMoveLeft ) m_BomberMove = BOMBERMOVE_LEFT;
            else if (DangerUp    == DANGER_NONE && m_BomberMove != BOMBERMOVE_DOWN  && CanMoveUp   ) m_BomberMove = BOMBERMOVE_UP;
            else                                                                                         m_BomberMove = BOMBERMOVE_NONE;
        }

        //WriteConsole ("BD - ");
    }
    else if (MarkDownLeft >= MarkDownRight &&
             MarkDownLeft >= MarkUpLeft &&
             MarkDownLeft >= MarkUpRight)
    {
        if (RANDOM(100) >= 50)
        {
                 if (DangerDown  == DANGER_NONE && m_BomberMove != BOMBERMOVE_UP    && CanMoveDown ) m_BomberMove = BOMBERMOVE_DOWN;
            else if (DangerLeft  == DANGER_NONE && m_BomberMove != BOMBERMOVE_RIGHT && CanMoveLeft ) m_BomberMove = BOMBERMOVE_LEFT;
            else if (DangerUp    == DANGER_NONE && m_BomberMove != BOMBERMOVE_DOWN  && CanMoveUp   ) m_BomberMove = BOMBERMOVE_UP;
            else if (DangerRight == DANGER_NONE && m_BomberMove != BOMBERMOVE_LEFT  && CanMoveRight) m_BomberMove = BOMBERMOVE_RIGHT;
            else                                                                                         m_BomberMove = BOMBERMOVE_NONE;
        }
        else
        {
                 if (DangerLeft  == DANGER_NONE && m_BomberMove != BOMBERMOVE_RIGHT && CanMoveLeft ) m_BomberMove = BOMBERMOVE_LEFT;
            else if (DangerDown  == DANGER_NONE && m_BomberMove != BOMBERMOVE_UP    && CanMoveDown ) m_BomberMove = BOMBERMOVE_DOWN;
            else if (DangerRight == DANGER_NONE && m_BomberMove != BOMBERMOVE_LEFT  && CanMoveRight) m_BomberMove = BOMBERMOVE_RIGHT;
            else if (DangerUp    == DANGER_NONE && m_BomberMove != BOMBERMOVE_DOWN  && CanMoveUp   ) m_BomberMove = BOMBERMOVE_UP;
            else                                                                                         m_BomberMove = BOMBERMOVE_NONE;
        }

        //WriteConsole ("BG - ");
    }
    else if (MarkUpLeft >= MarkDownRight &&
             MarkUpLeft >= MarkDownLeft &&
             MarkUpLeft >= MarkUpRight)
    {
        if (RANDOM(100) >= 50)
        {
                 if (DangerUp    == DANGER_NONE && m_BomberMove != BOMBERMOVE_DOWN  && CanMoveUp   ) m_BomberMove = BOMBERMOVE_UP;
            else if (DangerLeft  == DANGER_NONE && m_BomberMove != BOMBERMOVE_RIGHT && CanMoveLeft ) m_BomberMove = BOMBERMOVE_LEFT;
            else if (DangerDown  == DANGER_NONE && m_BomberMove != BOMBERMOVE_UP    && CanMoveDown ) m_BomberMove = BOMBERMOVE_DOWN;
            else if (DangerRight == DANGER_NONE && m_BomberMove != BOMBERMOVE_LEFT  && CanMoveRight) m_BomberMove = BOMBERMOVE_RIGHT;
            else                                                                                         m_BomberMove = BOMBERMOVE_NONE;
        }
        else
        {
                 if (DangerLeft  == DANGER_NONE && m_BomberMove != BOMBERMOVE_RIGHT && CanMoveLeft ) m_BomberMove = BOMBERMOVE_LEFT;
            else if (DangerUp    == DANGER_NONE && m_BomberMove != BOMBERMOVE_DOWN  && CanMoveUp   ) m_BomberMove = BOMBERMOVE_UP;
            else if (DangerRight == DANGER_NONE && m_BomberMove != BOMBERMOVE_LEFT  && CanMoveRight) m_BomberMove = BOMBERMOVE_RIGHT;
            else if (DangerDown  == DANGER_NONE && m_BomberMove != BOMBERMOVE_UP    && CanMoveDown ) m_BomberMove = BOMBERMOVE_DOWN;
            else                                                                                         m_BomberMove = BOMBERMOVE_NONE;
        }

        //WriteConsole ("HG - ");
    }
    else
    {
        if (RANDOM(100) >= 50)
        {
                 if (DangerUp    == DANGER_NONE && m_BomberMove != BOMBERMOVE_DOWN  && CanMoveUp   ) m_BomberMove = BOMBERMOVE_UP;
            else if (DangerRight == DANGER_NONE && m_BomberMove != BOMBERMOVE_LEFT  && CanMoveRight) m_BomberMove = BOMBERMOVE_RIGHT;
            else if (DangerDown  == DANGER_NONE && m_BomberMove != BOMBERMOVE_UP    && CanMoveDown ) m_BomberMove = BOMBERMOVE_DOWN;
            else if (DangerLeft  == DANGER_NONE && m_BomberMove != BOMBERMOVE_RIGHT && CanMoveLeft ) m_BomberMove = BOMBERMOVE_LEFT;
            else                                                                                         m_BomberMove = BOMBERMOVE_NONE;
        }
        else
        {
                 if (DangerRight == DANGER_NONE && m_BomberMove != BOMBERMOVE_LEFT  && CanMoveRight) m_BomberMove = BOMBERMOVE_RIGHT;
            else if (DangerUp    == DANGER_NONE && m_BomberMove != BOMBERMOVE_DOWN  && CanMoveUp   ) m_BomberMove = BOMBERMOVE_UP;
            else if (DangerLeft  == DANGER_NONE && m_BomberMove != BOMBERMOVE_RIGHT && CanMoveLeft ) m_BomberMove = BOMBERMOVE_LEFT;
            else if (DangerDown  == DANGER_NONE && m_BomberMove != BOMBERMOVE_UP    && CanMoveDown ) m_BomberMove = BOMBERMOVE_DOWN;
            else                                                                                         m_BomberMove = BOMBERMOVE_NONE;
        }

        //WriteConsole ("HD - ");
    }

    // If the bomber move is a real move
    if (m_BomberMove != BOMBERMOVE_NONE)
    {
        // Send this bomber move to the bomber as many seconds as the time
        // a bomber takes to walk through an entire block.
        m_BomberMoveTimeLeft = BLOCK_SIZE * 1.0f / m_pBomber->GetPixelsPerSecond();
    }
    // If the bomber move is not a real move
    else
    {
        m_BomberMoveTimeLeft = 0.0f;
    }

    m_WalkTime += DeltaTime;
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

bool CAiBomber::GoTo (int GoalBlockX, int GoalBlockY)
{
    // If the block to go to is not accessible 
    // or the bomber is already on this block
    if (m_Accessible[GoalBlockX][GoalBlockY] == -1 || 
        m_Accessible[GoalBlockX][GoalBlockY] == 0)
    {
        // Set no bomber move to send to the bomber
        m_BomberMove = BOMBERMOVE_NONE;
    }
    // If the block to go to is accessible and the bomber is not on this block
    else
    {   
        // Block coordinates used to go from the goal to the 
        // bomber using the accessible array.
        // Start from the goal.
        int BlockX = GoalBlockX;
        int BlockY = GoalBlockY;

        // While we have not reached the block where the bomber is
        // and determined what bomber move to set
        while (true)
        {
            // If going to the block above makes us go closer to the block where the bomber is
            if (m_Accessible[BlockX][BlockY-1] == m_Accessible[BlockX][BlockY] - 1)
            {
                // If the block above is the block where the bomber is
                if (m_Accessible[BlockX][BlockY-1] == 0)
                {
                    // We reached the bomber. Therefore the bomber has to go down.
                    m_BomberMove = BOMBERMOVE_DOWN;

                    // We're done with determining the bomber move to send to the bomber
                    break;
                }

                // Go up
                BlockY--;
            }
            // If going to the block below makes us go closer to the block where the bomber is
            else if (m_Accessible[BlockX][BlockY+1] == m_Accessible[BlockX][BlockY] - 1)
            {
                // If the block below is the block where the bomber is
                if (m_Accessible[BlockX][BlockY+1] == 0)
                {
                    // We reached the bomber. Therefore the bomber has to go up.
                    m_BomberMove = BOMBERMOVE_UP;

                    // We're done with determining the bomber move to send to the bomber
                    break;
                }

                // Go down
                BlockY++;
            }
            // If going to the block to the left makes us go closer to the block where the bomber is
            else if (m_Accessible[BlockX-1][BlockY] == m_Accessible[BlockX][BlockY] - 1)
            {
                // If the block to the left is the block where the bomber is
                if (m_Accessible[BlockX-1][BlockY] == 0)
                {
                    // We reached the bomber. Therefore the bomber has to go right.
                    m_BomberMove = BOMBERMOVE_RIGHT;

                    // We're done with determining the bomber move to send to the bomber
                    break;
                }

                // Go left
                BlockX--;
            }
            // If going to the block to the right makes us go closer to the block where the bomber is
            else if (m_Accessible[BlockX+1][BlockY] == m_Accessible[BlockX][BlockY] - 1)
            {
                // If the block to the right is the block where the bomber is
                if (m_Accessible[BlockX+1][BlockY] == 0)
                {
                    // We reached the bomber. Therefore the bomber has to go left.
                    m_BomberMove = BOMBERMOVE_LEFT;

                    // We're done with determining the bomber move to send to the bomber
                    break;
                }

                // Go right
                BlockX++;
            }
        } // while

        if (m_pArena->m_pArena->GetArenaCloser().GetNumberOfBlocksLeft() > 10)
        {
            float DangerTimeLeftHere = m_pArena->m_DangerTimeLeft[m_BlockHereX][m_BlockHereY];
            float DangerTimeLeftNext = -1.0f;

            switch (m_BomberMove)
            {
                case BOMBERMOVE_UP    : DangerTimeLeftNext = m_pArena->m_DangerTimeLeft[m_BlockUpX][m_BlockUpY]; break;
                case BOMBERMOVE_DOWN  : DangerTimeLeftNext = m_pArena->m_DangerTimeLeft[m_BlockDownX][m_BlockDownY]; break;
                case BOMBERMOVE_LEFT  : DangerTimeLeftNext = m_pArena->m_DangerTimeLeft[m_BlockLeftX][m_BlockLeftY]; break;
                case BOMBERMOVE_RIGHT : DangerTimeLeftNext = m_pArena->m_DangerTimeLeft[m_BlockRightX][m_BlockRightY]; break;
                default: break;
            }

            ASSERT (DangerTimeLeftNext != -1.0f);

            if (DangerTimeLeftNext == 0.0f || DangerTimeLeftHere - DangerTimeLeftNext >= 0.500f)
            {
                m_BomberMove = BOMBERMOVE_NONE;
            }
        }
    }

    // If the bomber move is a real move
    if (m_BomberMove != BOMBERMOVE_NONE)
    {
        // Send this bomber move to the bomber as many seconds as the time
        // a bomber takes to walk through an entire block.
        m_BomberMoveTimeLeft = BLOCK_SIZE * 1.0f / m_pBomber->GetPixelsPerSecond();
    }
    // If the bomber move is not a real move
    else
    {
        // Send this bomber move only on this update
        m_BomberMoveTimeLeft = 0.0f;
    }

    // Return whether the bomber is on the goal block
    return m_Accessible[GoalBlockX][GoalBlockY] == 0;
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

void CAiBomber::SetComputerMode (EComputerMode ComputerMode)
{
    // If we are switching to think mode
    if (ComputerMode == COMPUTERMODE_THINK)
    {
        // Stop commanding the bomber for a little time
        m_StopTimeLeft = 0.180f;

        switch (m_ComputerMode)
        {
            case COMPUTERMODE_ITEM    : m_StopTimeLeft = 0.080f + RANDOM(40) / 1000.0f; break;
            case COMPUTERMODE_ATTACK  : m_StopTimeLeft = 0.200f + RANDOM(40) / 1000.0f; break;
            case COMPUTERMODE_DEFENCE : m_StopTimeLeft = 0.120f + RANDOM(40) / 1000.0f; break;
            case COMPUTERMODE_WALK    : m_StopTimeLeft = 0.220f + RANDOM(40) / 1000.0f; break;
            default: break;
        }
    }

    // Set the new computer mode
    m_ComputerMode = ComputerMode;
/*
    // Write a console line to specify the mode switch
    switch (m_ComputerMode)
    {
        case COMPUTERMODE_THINK   : WriteConsole ("MODE THINK\n");   break;
        case COMPUTERMODE_ITEM    : WriteConsole ("MODE ITEM\n");    break;
        case COMPUTERMODE_ATTACK  : WriteConsole ("MODE ATTACK\n");  break;
        case COMPUTERMODE_DEFENCE : WriteConsole ("MODE DEFENCE\n"); break;
        case COMPUTERMODE_WALK    : WriteConsole ("MODE WALK\n");    break;
    }*/
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************

void CAiBomber::UpdateAccessibility()
{
    //****************
    // ACCESSIBLE
    //****************

    int BlockX;
    int BlockY;

    bool Updated;

    // All squares are assumed inaccessible
    for (BlockX = 0 ; BlockX < ARENA_WIDTH ; BlockX++)
    {
        for (BlockY = 0 ; BlockY < ARENA_HEIGHT ; BlockY++)
        {
            m_Accessible[BlockX][BlockY] = -1;
        }
    }

    // The square where the bomber is is accessible (distance zero)
    m_Accessible[m_BlockHereX][m_BlockHereY] = 0;

    m_NumAccessible = 1;

    // Find all the squares that are accessible to the bomber
    // If no update was performed then it's over because there 
    // are no more accessible squares that weren't marked.
    do
    {
        Updated = false;

        // Scan each block of the AI view
        for (BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
        {
            for (BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
            {
                // If the block is on the edges of the arena grid
                if (BlockX <= 0 || BlockX >= ARENA_WIDTH - 1 ||
                    BlockY <= 0 || BlockY >= ARENA_HEIGHT - 1)
                {
                    continue;
                }
                // If it is an accessible square 
                else if (m_Accessible[BlockX][BlockY] != -1)
                {
                    // If there is a square ABOVE that is marked as not accessible and it's not a wall/bomb
                    if (m_Accessible[BlockX][BlockY-1] == -1 && 
                        !m_pArena->m_pArena->IsWall(BlockX,BlockY-1) &&
                        !m_pArena->m_pArena->IsBomb(BlockX,BlockY-1))
                    {
                        // Mark it as accessible
                        m_Accessible[BlockX][BlockY-1] = m_Accessible[BlockX][BlockY] + 1;
                        
                        Updated = true;

                        m_NumAccessible++;
                    }

                    // If there is a square BELOW that is marked as not accessible and it's not a wall/bomb
                    if (m_Accessible[BlockX][BlockY+1] == -1 && 
                        !m_pArena->m_pArena->IsWall(BlockX,BlockY+1) &&
                        !m_pArena->m_pArena->IsBomb(BlockX,BlockY+1))
                    {
                        // Mark it as accessible
                        m_Accessible[BlockX][BlockY+1] = m_Accessible[BlockX][BlockY] + 1;
                        
                        Updated = true;

                        m_NumAccessible++;
                    }

                    // If there is a square TO THE LEFT that is marked as not accessible and it's not a wall/bomb
                    if (m_Accessible[BlockX-1][BlockY] == -1 && 
                        !m_pArena->m_pArena->IsWall(BlockX-1,BlockY) &&
                        !m_pArena->m_pArena->IsBomb(BlockX-1,BlockY))
                    {
                        // Mark it as accessible
                        m_Accessible[BlockX-1][BlockY] = m_Accessible[BlockX][BlockY] + 1;
                        
                        Updated = true;

                        m_NumAccessible++;
                    }

                    // If there is a square TO THE RIGHT that is marked as not accessible and it's not a wall/bomb
                    if (m_Accessible[BlockX+1][BlockY] == -1 && 
                        !m_pArena->m_pArena->IsWall(BlockX+1,BlockY) &&
                        !m_pArena->m_pArena->IsBomb(BlockX+1,BlockY))
                    {
                        // Mark it as accessible
                        m_Accessible[BlockX+1][BlockY] = m_Accessible[BlockX][BlockY] + 1;
                        
                        Updated = true;

                        m_NumAccessible++;
                    }
                }
            }
        }
    } 
    while (Updated);

    //****************
    // PSEUDO ACCESSIBLE
    //****************

    // All squares are assumed inaccessible
    for (BlockX = 0 ; BlockX < ARENA_WIDTH ; BlockX++)
    {
        for (BlockY = 0 ; BlockY < ARENA_HEIGHT ; BlockY++)
        {
            m_PseudoAccessible[BlockX][BlockY] = -1;
        }
    }

    // The square where the bomber is is accessible (distance zero)
    m_PseudoAccessible[m_BlockHereX][m_BlockHereY] = 0;

    m_NumAccessible = 1;

    // Find all the squares that are accessible to the bomber
    // If no update was performed then it's over because there 
    // are no more accessible squares that weren't marked.
    do
    {
        Updated = false;

        // Scan each block of the AI view
        for (BlockX = m_BlockHereX - AI_VIEW_SIZE ; BlockX < m_BlockHereX + AI_VIEW_SIZE ; BlockX++)
        {
            for (BlockY = m_BlockHereY - AI_VIEW_SIZE ; BlockY < m_BlockHereY + AI_VIEW_SIZE ; BlockY++)
            {
                // If the block is on the edges of the arena grid
                if (BlockX <= 0 || BlockX >= ARENA_WIDTH - 1 ||
                    BlockY <= 0 || BlockY >= ARENA_HEIGHT - 1)
                {
                    continue;
                }
                // If it is an accessible square 
                else if (m_PseudoAccessible[BlockX][BlockY] != -1)
                {
                    // If there is a square ABOVE that is marked as not accessible and it's not a wall/bomb
                    if (m_PseudoAccessible[BlockX][BlockY-1] == -1 && 
                        !m_pArena->m_pArena->IsWall(BlockX,BlockY-1))
                    {
                        // Mark it as accessible
                        m_PseudoAccessible[BlockX][BlockY-1] = m_PseudoAccessible[BlockX][BlockY] + 1;
                        
                        Updated = true;

                        m_NumAccessible++;
                    }

                    // If there is a square BELOW that is marked as not accessible and it's not a wall/bomb
                    if (m_PseudoAccessible[BlockX][BlockY+1] == -1 && 
                        !m_pArena->m_pArena->IsWall(BlockX,BlockY+1))
                    {
                        // Mark it as accessible
                        m_PseudoAccessible[BlockX][BlockY+1] = m_PseudoAccessible[BlockX][BlockY] + 1;
                        
                        Updated = true;

                        m_NumAccessible++;
                    }

                    // If there is a square TO THE LEFT that is marked as not accessible and it's not a wall/bomb
                    if (m_PseudoAccessible[BlockX-1][BlockY] == -1 && 
                        !m_pArena->m_pArena->IsWall(BlockX-1,BlockY))
                    {
                        // Mark it as accessible
                        m_PseudoAccessible[BlockX-1][BlockY] = m_PseudoAccessible[BlockX][BlockY] + 1;
                        
                        Updated = true;

                        m_NumAccessible++;
                    }

                    // If there is a square TO THE RIGHT that is marked as not accessible and it's not a wall/bomb
                    if (m_PseudoAccessible[BlockX+1][BlockY] == -1 && 
                        !m_pArena->m_pArena->IsWall(BlockX+1,BlockY))
                    {
                        // Mark it as accessible
                        m_PseudoAccessible[BlockX+1][BlockY] = m_PseudoAccessible[BlockX][BlockY] + 1;
                        
                        Updated = true;

                        m_NumAccessible++;
                    }
                }
            }
        }
    } 
    while (Updated);
}

//******************************************************************************************************************************
//******************************************************************************************************************************
//******************************************************************************************************************************
