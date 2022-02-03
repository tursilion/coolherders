using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using CoolHerders.Housekeeping;
using CoolHerders.Pathfinder;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Audio;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Microsoft.Xna.Framework.Net;

namespace CoolHerders.GameItems
{
    class NetworkPlayerCharacter : PlayerCharacter
    {
        private Vector2 networkCharacterPosition;
        private float networkCharacterSpeed;
        private Direction networkCharacterDirection;
        private bool networkCharacterStopped;
        private bool networkPacketsReceived;
        private int networkDestinationColumn;
        private int networkDestinationRow;

        /// <summary>
        /// The list of positions returned from the pathfinder through which the herder must walk
        /// </summary>
        private List<MazeCoordinate> walkingList = null;

        /// <summary>
        /// The position within the walkingList which the herder is heading towards
        /// </summary>
        private int walkingListPosition = 0;

        public NetworkPlayerCharacter(MazeGameComponent parentComponent, NetworkGamer networkGamer, PlayerInformation playerInformation, int rowCounter, int columnCounter)
            : base(parentComponent, networkGamer, playerInformation, rowCounter, columnCounter)
        {
            networkPacketsReceived = false;
        }

        internal override void Update(GameTime gameTime, ICollidable[] obstructingObjects, ICollidable[] collectableObjects)
        {
            if (networkPacketsReceived)
            {
                base.Update(gameTime, obstructingObjects, collectableObjects);
            }
        }

        public override void SetCharacterSpeed()
        {
        }

        protected override void HandleMovementChange(PlayerIndex playerIndex, bool atMazeJunction, ICollidable[] collectableObjects)
        {
            if (atMazeJunction)
            {
                if ((networkDestinationColumn != 0) && (networkDestinationRow != 0))
                {
                    // We have received a valid destination from the network
                    if ((networkDestinationColumn != destinationColumn) || (networkDestinationRow != destinationRow))
                    {
                        // We have not yet seen this destination
                        destinationColumn = networkDestinationColumn;
                        destinationRow = networkDestinationRow;

                        bool inSync = false;
                        if ((networkCharacterDirection == Direction.DirectUp) && (networkDestinationColumn == this.CharacterColumn) && (networkDestinationRow == (this.CharacterRow - 1)))
                        {
                            inSync = true;
                        }
                        else if ((networkCharacterDirection == Direction.DirectDown) && (networkDestinationColumn == this.CharacterColumn) && (networkDestinationRow == (this.CharacterRow + 1)))
                        {
                            inSync = true;
                        }
                        else if ((networkCharacterDirection == Direction.DirectLeft) && (networkDestinationColumn == (this.CharacterColumn - 1)) && (networkDestinationRow == this.CharacterRow))
                        {
                            inSync = true;
                        }
                        else if ((networkCharacterDirection == Direction.DirectRight) && (networkDestinationColumn == (this.CharacterColumn + 1)) && (networkDestinationRow == this.CharacterRow))
                        {
                            inSync = true;
                        }

                        if (!inSync)
                        {
                            MazeCoordinate destinationCoordinate = AStarPathfinder.MazePool.Fetch();
                            destinationCoordinate.ResetMazeCoordinate(networkDestinationColumn, networkDestinationRow);
                            FindAPath(destinationCoordinate);
                        }
                        else
                        {
                            if ((((networkCharacterDirection == Direction.DirectUp) || (networkCharacterDirection == Direction.DirectDown)) &&
                                 ((this.LastDirection == Direction.DirectLeft) || (this.LastDirection == Direction.DirectRight))) ||
                                (((networkCharacterDirection == Direction.DirectLeft) || (networkCharacterDirection == Direction.DirectRight)) &&
                                 ((this.LastDirection == Direction.DirectUp) || (this.LastDirection == Direction.DirectDown))))
                            {
                                // If we must make a turn, then let's snap the player to the grid
                                this.CharacterRow = (int)Math.Round(this.CharacterPosition.Y / 32);
                                this.CharacterColumn = (int)Math.Round(this.CharacterPosition.X / 32);
                                this.CharacterPosition = new Vector2(this.CharacterColumn * 32, this.CharacterRow * 32);
                            }

                            this.LastDirection = networkCharacterDirection;
                            this.CharacterSpeed = networkCharacterSpeed;
                            this.characterStopped = networkCharacterStopped;
                        }
                    }
                }

                if (null != this.walkingList)
                {
                    if (this.walkingListPosition == this.walkingList.Count)
                    {
                        this.characterStopped = true;
                        return;
                    }

                    GetDirectionFromGridDifference(this.CharacterColumn, this.CharacterRow, this.walkingList[this.walkingListPosition]);

                    if (CanGoForward())
                    {
                        this.characterStopped = false;
                        this.walkingListPosition++;
                    }
                }
           }
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

        public void DecodeNetworkPacket()
        {
            byte packetIdentifier = GameInformation.Instance.GamePacketReader.ReadByte();
            byte movementIndicator = GameInformation.Instance.GamePacketReader.ReadByte();
            float charSpeed = GameInformation.Instance.GamePacketReader.ReadSingle();
            int destCol = GameInformation.Instance.GamePacketReader.ReadByte();
            int destRow = GameInformation.Instance.GamePacketReader.ReadByte();

            networkPacketsReceived = true;

            networkCharacterDirection = (Direction)(movementIndicator & 0x7F);
            networkCharacterSpeed = charSpeed;
            networkCharacterStopped = ((movementIndicator & 0x80) == 0x80);
            networkDestinationColumn = destCol;
            networkDestinationRow = destRow;
        }
    }
}