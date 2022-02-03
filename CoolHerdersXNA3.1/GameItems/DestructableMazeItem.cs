//-----------------------------------------------------------------------------
// <copyright file="DestructableMazeItem.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    /// This class describes a maze item that can be destroyed during the course of the game
    /// </summary>
    internal class DestructableMazeItem : MazeItem
    {
        /// <summary>
        /// The pickup item, if any, that is hidden wthin this box
        /// </summary>
        public PickupItem HiddenItem;

        /// <summary>
        /// The time per frame of destruction in milliseconds
        /// </summary>
        private static readonly TimeSpan destructAnimationTime = TimeSpan.FromMilliseconds(150);

        /// <summary>
        /// Is this thing actively being destroyed.
        /// </summary>
        private bool isDestroying;

        /// <summary>
        /// This tracks the time the destruction animation has been running
        /// </summary>
        private TimeSpan destructTime = destructAnimationTime;

        /// <summary>
        /// The state of the destruction
        /// </summary>
        private int destructState;

        /// <summary>
        /// Initializes a new instance of the DestructableMazeItem class
        /// </summary>
        /// <param name="parentComponent">The parent component on which this maze item will appear</param>
        /// <param name="resourceDirectory">The resource directory in which to find the tiles for this item</param>
        /// <param name="levelNumber">The level number which we are drawing</param>
        /// <param name="mazeTile">The maze tile which we are drawing</param>
        /// <param name="rowCounter">The row of the maze this tile appears on</param>
        /// <param name="columnCounter">The column of the maze this tile appears on</param>
        public DestructableMazeItem(MazeGameComponent parentComponent, string resourceDirectory, int levelNumber, MazeMap.MazeTile mazeTile, int rowCounter, int columnCounter) : base(parentComponent, resourceDirectory, levelNumber, mazeTile, rowCounter, columnCounter)
        {
            this.itemIsDestructible = true;

            MazeMap.MazeTile tempTile;
            int tileset = mazeTile.UnderneathTileSet;
            int tilenum = mazeTile.UnderneathImageNumber;
            tempTile = new MazeMap.MazeTile(tileset, tilenum, true, false, false, 0, 0);

            this.underMazeItem = new MazeItem(parentComponent, resourceDirectory, levelNumber, tempTile, rowCounter, columnCounter);

            this.itemDepth = (rowCounter * 0.05f) + (columnCounter * 0.002f);

            this.HiddenItem = null;
        }

        /// <summary>
        /// Cause the object to begin destroying itself on every frame
        /// </summary>
        public void StartDestruction()
        {
            if (0 == this.destructState)
            {
                this.itemTextureLocation.X += 48;
                this.destructState = 1;
                this.isDestroying = true;
            }
        }

        /// <summary>
        /// Updates the object's state in the game world
        /// </summary>
        /// <param name="gameTime">The current GameTime for this game</param>
        public override void Update(Microsoft.Xna.Framework.GameTime gameTime)
        {
            base.Update(gameTime);

            if (this.isDestroying)
            {
                this.destructTime -= gameTime.ElapsedGameTime;
                if (this.destructTime.TotalSeconds <= 0)
                {
                    this.destructState++;
                    this.itemTextureLocation.X += 48;
                    if (this.destructState < 4)
                    {
                        this.destructTime = this.destructTime.Add(destructAnimationTime);
                    }
                    else
                    {
                        this.itemDepth = 0.00001f;
                        this.destructTime = TimeSpan.Zero;
                        this.isDestroying = false;
                        this.itemIsPassable = true;
                        this.itemIsDestructible = false;
                        if (this.HiddenItem != null)
                        {
                            this.ParentComponent.PickupItems.Add(this.HiddenItem);
                            this.HiddenItem = null;
                        }
                    }
                }
            }
        }
    }
}
