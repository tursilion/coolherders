//-----------------------------------------------------------------------------
// <copyright file="EnemyHerderCharacter.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text;
    using CoolHerders.GameItems;
    using CoolHerders.Pathfinder;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;

    /// <summary>
    /// This tracks the enemy herder character as it moves around the maze
    /// </summary>
    internal class EnemyHerderCharacter : PlayerCharacter
    {
        /// <summary>
        /// How long should a player's controller vibrate for
        /// </summary>
        private static readonly TimeSpan vibrationDuration = TimeSpan.FromMilliseconds(250);

        /// <summary>
        /// A random number source for selecting the next action to perform
        /// </summary>
        private Random randomNumberSource = new Random();

        /// <summary>
        /// Does the pathfinding algorithm need to be tickled on the next pass
        /// </summary>
        private bool testPathfinding = true;

        /// <summary>
        /// The list of positions returned from the pathfinder through which the herder must walk
        /// </summary>
        private List<MazeCoordinate> walkingList = null;

        /// <summary>
        /// The position within the walkingList which the herder is heading towards
        /// </summary>
        private int walkingListPosition = 0;
        
        /// <summary>
        /// The power contained within the zapper at the moment the herder began firing
        /// </summary>
        private int zapperPowerAtFiring = 0;

        /// <summary>
        /// A specific coordinate to be selected for pathfinding, skipping the target selection process
        /// </summary>
        private MazeCoordinate forceMazeTarget = null;

        /// <summary>
        /// Initializes a new instance of the EnemyHerderCharacter class.
        /// </summary>
        /// <param name="parentComponent">The parent component on which this class will be drawn</param>
        /// <param name="gamerClass">The class holding the gamer information, or the owning player info if a computer managed creation</param>
        /// <param name="playerClassName">The type of player we are creating, by name</param>
        /// <param name="subclass">The subclass name for this player</param>
        /// <param name="rowCounter">The starting row of this player</param>
        /// <param name="columnCounter">The starting column of this player</param>
        public EnemyHerderCharacter(MazeGameComponent parentComponent, NetworkGamer networkGamer, PlayerInformation playerInformation, int rowCounter, int columnCounter)
            : base(parentComponent, networkGamer, playerInformation, rowCounter, columnCounter)
        {
            canDestroyBoxes = true;
        }

        /// <summary>
        /// Finalizes an instance of the EnemyHerderCharacter class
        /// </summary>
        ~EnemyHerderCharacter()
        {
            this.ReleaseWalkingList();
        }

        /// <summary>
        /// Called when the enemy herder is to drop a sheep
        /// </summary>
        /// <param name="attackingPlayer">The player that attacked us to cause this</param>
        /// <returns>True if a sheep was actually dropped, false if we weren't carrying one to begin with</returns>
        public override bool DropSheep(PlayerCharacter attackingPlayer)
        {
            this.ReleaseWalkingList();
            this.walkingListPosition = 0;
            this.testPathfinding = true;
            this.characterStopped = true;
            return base.DropSheep(attackingPlayer);
        }

        /// <summary>
        /// Updates the player character
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        /// <param name="obstructingObjects">Objects which we may not move through</param>
        /// <param name="collectableObjects">Objects which we may collect</param>
        internal override void Update(GameTime gameTime, ICollidable[] obstructingObjects, ICollidable[] collectableObjects)
        {
            Vector2 previousCharPosition = this.CharacterPosition;

            if (this.PlayerZapper.CurrentLength == 0)
            {
                this.PlayerZapper.StopFiring();
            }

            int maximumLength = this.PlayerZapper.FindMaxLength(false);
            float shootableThingDistance = float.MaxValue;

            foreach (PlayerCharacter player in obstructingObjects)
            {
                if (null != player)
                {
                    if (player != this)
                    {
                        float? characterDistance = player.CheckCollision(this.PlayerZapper.CollisionRay);
                        if ((characterDistance != null) && (characterDistance < maximumLength * 32.0f))
                        {
                            if (characterDistance < shootableThingDistance)
                            {
                                shootableThingDistance = (float)characterDistance;
                            }
                        }
                    }
                }
            }

            if (null != collectableObjects)
            {
                foreach (SheepCharacter sheepie in collectableObjects)
                {
                    if (null != sheepie)
                    {
                        float? characterDistance = sheepie.CheckCollision(this.PlayerZapper.CollisionRay);
                        if ((characterDistance != null) && (characterDistance < maximumLength * 32.0f))
                        {
                            if (characterDistance < shootableThingDistance)
                            {
                                shootableThingDistance = (float)characterDistance;
                            }
                        }
                    }
                }
            }

            if (this.playerInformation.SkillLevel > 0)
            {
                // Herders of skill level 0 don't zap things
                if (shootableThingDistance < float.MaxValue)
                {
                    // We found something to shoot, now...can we?
                    if (shootableThingDistance < this.PlayerZapper.ZapperPowerToTiles() * 32.0f)
                    {
                        int shootProbability = randomNumberSource.Next(1, 10);
                        if (this.playerInformation.SkillLevel >= shootProbability)
                        {
                            // Yes, we have enough power to zap it
                            if (this.checkZapperCollisions)
                            {
                                this.PlayerZapper.FireZapper();
                                this.zapperPowerAtFiring = this.PlayerZapper.ZapperPower;
                            }
                        }
                    }
                }
            }

            base.Update(gameTime, obstructingObjects, collectableObjects);
        }

        /// <summary>
        /// Handle a collision with a movable object which we cannot pick up or destroy
        /// </summary>
        protected override void HandleObstructingCollision()
        {
            if (!this.beingShoved)
            {
                Vector2 centerPosition = new Vector2(this.CharacterColumn * 32.0f, this.CharacterRow * 32.0f);
                if (Vector2.DistanceSquared(this.CharacterPosition, centerPosition) < 9.0f)
                {
                    this.CharacterPosition = centerPosition;
                }
                else
                {
                    TurnCharacterReverse();
                }

                this.testPathfinding = true;
            }
        }
        
        /// <summary>
        /// Called when the enemy herder needs to change direction.
        /// </summary>
        /// <param name="playerIndex">The player index that is controlling us (presently unused)</param>
        /// <param name="atMazeJunction">Is this herder at a maze junction right now</param>
        /// <param name="collectableObjects">Items within the maze that can be picked up by this herder</param>
        protected override void HandleMovementChange(PlayerIndex playerIndex, bool atMazeJunction, ICollidable[] collectableObjects)
        {
            if (this.beingShoved)
            {
                return;
            }

            if (!atMazeJunction)
            {
                return;
            }

            this.characterStopped = true;

            if (this.testPathfinding)
            {
                MazeCoordinate ourTarget;
                if (this.forceMazeTarget != null)
                {
                    ourTarget = this.forceMazeTarget;
                    this.forceMazeTarget = null;
                }
                else
                {
                    ourTarget = this.SelectNextTarget(collectableObjects);
                }

                this.FindAPath(ourTarget);
                if (this.walkingList != null)
                {
                    this.testPathfinding = false;
                }
                else
                {
                    this.characterStopped = true;
                }
            }

            if (null != this.walkingList)
            {
                if (this.walkingListPosition == this.walkingList.Count)
                {
                    this.testPathfinding = true;
                    return;
                }

                GetDirectionFromGridDifference(this.CharacterColumn, this.CharacterRow, this.walkingList[this.walkingListPosition]);

                if (CanGoForward())
                {
                    this.characterStopped = false;
                    this.walkingListPosition++;
                }
                else
                {
                    if (CanDestroyForward() && this.checkZapperCollisions)
                    {
                        if ((!this.PlayerZapper.ZapperOn) && (this.PlayerZapper.ZapperPower > 25)) 
                        {
                            this.PlayerZapper.FireZapper();
                            this.zapperPowerAtFiring = this.PlayerZapper.ZapperPower;
                        }
                    }
                    else
                    {
                        this.testPathfinding = true;
                    }
                }
            }
        }

        /// <summary>
        /// Target selection routine
        /// </summary>
        /// <param name="collectableObjects">A list of collectable objects we might want to head for</param>
        /// <returns>A maze coordinate to head for</returns>
        protected MazeCoordinate SelectNextTarget(ICollidable[] collectableObjects)
        {
            ICollidable targetCharacter;
            SheepCharacter sheepTarget;
            int escapeCounter = 0;

            if (collectableObjects.Length < 1)
            {
                return null;        // too early
            }

            int sheepNumber = this.randomNumberSource.Next(collectableObjects.Length - 1);
            int sheepTargetProbability = this.randomNumberSource.Next(100);
            int sheepTargetSkill = (this.playerInformation.SkillLevel * 5) + 50;
            if (sheepTargetProbability < sheepTargetSkill)
            {
                do
                {
                    targetCharacter = collectableObjects[sheepNumber];
                    sheepTarget = targetCharacter as SheepCharacter;
                    sheepNumber = (sheepNumber + 1) % collectableObjects.Length;
                    escapeCounter++;
                    if (escapeCounter == 100)
                    {
                        MazeCoordinate mazeTarget = AStarPathfinder.MazePool.Fetch();
                        MazeItem item = this.parentComponent.PassableMazeItems[this.randomNumberSource.Next(this.parentComponent.PassableMazeItems.Count - 1)];
                        mazeTarget.ResetMazeCoordinate(item.ColumnPosition, item.RowPosition);
                        return mazeTarget;
                    }
                }
                while ((sheepTarget == null) || (sheepTarget.OwningPlayer != null));
            }
            else
            {
                MazeCoordinate mazeTarget = AStarPathfinder.MazePool.Fetch();
                MazeItem item = this.parentComponent.PassableMazeItems[this.randomNumberSource.Next(this.parentComponent.PassableMazeItems.Count - 1)];
                mazeTarget.ResetMazeCoordinate(item.ColumnPosition, item.RowPosition);
                return mazeTarget;
            }

            MazeCoordinate newTarget = null;

            newTarget = AStarPathfinder.MazePool.Fetch();
            newTarget.ResetMazeCoordinate(sheepTarget.CharacterColumn, sheepTarget.CharacterRow);
            return newTarget;
        }

        /// <summary>
        /// Given the current location of the herder, plots a path to a given destination
        /// Only valid to call this if we are even with a tile
        /// </summary>
        /// <param name="destinationCoordinate">The destination coordinate to route to</param>
        protected void FindAPath(MazeCoordinate destinationCoordinate)
        {
            MazeCoordinate sourceCoordinate = AStarPathfinder.MazePool.Fetch();
            sourceCoordinate.ResetMazeCoordinate(this.CharacterColumn, this.CharacterRow);
            if ((sourceCoordinate == null) || (destinationCoordinate == null))
            {
                this.ReleaseWalkingList();
                AStarPathfinder.MazePool.Release(sourceCoordinate);
                AStarPathfinder.MazePool.Release(destinationCoordinate);
                return;
            }

            if ((sourceCoordinate.CellColumn == destinationCoordinate.CellColumn) && (sourceCoordinate.CellRow == destinationCoordinate.CellRow))
            {
                this.ReleaseWalkingList();
                AStarPathfinder.MazePool.Release(sourceCoordinate);
                AStarPathfinder.MazePool.Release(destinationCoordinate);
                return;
            }

            this.ReleaseWalkingList();
            AStarPathfinder path = new AStarPathfinder(this.parentComponent);
            this.walkingList = path.ComputePath(sourceCoordinate, destinationCoordinate);
            if (this.walkingList == null)
            {
                return;
            }

            this.walkingListPosition = 0;
        }

        /// <summary>
        /// Called when an enemy herder touches a sheep
        /// </summary>
        /// <param name="sheepie">The sheep that was touched</param>
        protected override void HandleSheepTouch(SheepCharacter sheepie)
        {
            base.HandleSheepTouch(sheepie);
            this.testPathfinding = true;
        }

        /// <summary>
        /// Called when the shove routine is done
        /// </summary>
        protected override void FinishBeingShoved()
        {
            base.FinishBeingShoved();
            this.characterStopped = false;
        }

        /// <summary>
        /// Called when this herder has touched something with it's zapper
        /// </summary>
        protected override void HandleZappedSomething()
        {
            base.HandleZappedSomething();
            if (this.PlayerZapper.ZapperPower < this.zapperPowerAtFiring - 50)
            {
                this.PlayerZapper.StopFiring();
            }
        }

        /// <summary>
        /// Called when this herder has stunned a sheep with it's zapper
        /// </summary>
        /// <param name="sheepColumn">The column in which the sheep was found</param>
        /// <param name="sheepRow">The row in which the sheep was found</param>
        protected override void HandleZappedSheep(int sheepColumn, int sheepRow)
        {
            base.HandleZappedSheep(sheepColumn, sheepRow);
            if (this.PlayerZapper.ZapperPower < this.zapperPowerAtFiring - 50)
            {
                this.PlayerZapper.StopFiring();
            }

            if (this.forceMazeTarget != null)
            {
                AStarPathfinder.MazePool.Release(this.forceMazeTarget);
            }

            MazeCoordinate mazeTarget = AStarPathfinder.MazePool.Fetch();
            mazeTarget.ResetMazeCoordinate(sheepColumn, sheepRow);
            this.forceMazeTarget = mazeTarget;
            this.testPathfinding = true;
        }

        /// <summary>
        /// Releases the list of tiles to walk for the path back to the pool
        /// </summary>
        private void ReleaseWalkingList()
        {
            if (this.walkingList != null)
            {
                foreach (MazeCoordinate mazeCoord in this.walkingList)
                {
                    AStarPathfinder.MazePool.Release(mazeCoord);
                }

                this.walkingList = null;
            }
        }
    }
}
