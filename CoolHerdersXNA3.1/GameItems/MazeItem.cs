//-----------------------------------------------------------------------------
// <copyright file="MazeItem.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------
namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Text;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Graphics;

    /// <summary>
    /// Holds the info for and draws a given item within the maze.  This can be subclassed
    /// </summary>
    public class MazeItem
    {
        /// <summary>
        /// The row in which this maze item exists
        /// </summary>
        public int RowPosition;

        /// <summary>
        /// The column in which this maze item exists
        /// </summary>
        public int ColumnPosition;

        /// <summary>
        /// Can sheep reach this tile without going through any destrucable spaces
        /// </summary>
        public bool SheepCanWalk = false;

        /// <summary>
        /// The game component which is managing this maze item
        /// </summary>
        internal MazeGameComponent ParentComponent;

        /// <summary>
        /// Holds the location of the item within the texture page
        /// </summary>
        protected Rectangle itemTextureLocation;

        /// <summary>
        /// Set to true if the tile is passable
        /// </summary>
        protected bool itemIsPassable;

        /// <summary>
        /// Set to true if the tile is destructible
        /// </summary>
        protected bool itemIsDestructible;

        /// <summary>
        /// Stores the Z-order, if you will, of the tile for the needs of the sprite engine
        /// </summary>
        protected float itemDepth;

        /// <summary>
        /// If we are destructable, there is a tile underneath ours
        /// </summary>
        protected MazeItem underMazeItem;

        /// <summary>
        /// Sets the number of steps in the animation cycle
        /// </summary>
        private const int AnimationSteps = 4;

        /// <summary>
        /// Holds the texture to use for this maze item.
        /// This should be a shared reference between many items, so please use a shared content manager
        /// Otherwise you will probably run the 360 out of memory
        /// </summary>
        private Texture2D[] mazeItemTextures;

        /// <summary>
        /// Holds the position of the item on the screen
        /// </summary>
        private Vector2 itemPosition;

        /// <summary>
        /// Holds the origin of the item texture, the point from which drawing and rotation should commence
        /// </summary>
        private Vector2 itemOrigin;

        /// <summary>
        /// This tracks the time the animation has been running
        /// </summary>
        private TimeSpan animationTime = TimeSpan.FromMilliseconds(250);

        /// <summary>
        /// The state of the background animation
        /// </summary>
        private int animationState;

        /// <summary>
        /// Initializes a new instance of the MazeItem class.
        /// </summary>
        /// <param name="parentComponent">The component which is handling our drawing and lifespan</param>
        /// <param name="resourceDirectory">The content subdirectory in which we can find all our resources</param>
        /// <param name="levelNumber">The level number we are on</param>
        /// <param name="mazeTile">The maze tile representing this tile</param>
        /// <param name="rowCounter">The row we are drawing at, in maze rows</param>
        /// <param name="columnCounter">The column we are drawing at, in maze columns</param>
        internal MazeItem(MazeGameComponent parentComponent, string resourceDirectory, int levelNumber, MazeMap.MazeTile mazeTile, int rowCounter, int columnCounter)
        {
            if ((rowCounter < 0) || (rowCounter > 16))
            {
                throw new ArgumentOutOfRangeException("rowCounter", "Row counter must be between 0 and 15");
            }

            if ((columnCounter < 0) || (columnCounter > 22))
            {
                throw new ArgumentOutOfRangeException("columnCounter", "Column counter must be between 0 and 21");
            }

            this.itemPosition = new Vector2((columnCounter * 32) - 8, (rowCounter * 32) - 15);

            this.ColumnPosition = columnCounter;
            this.RowPosition = rowCounter;

            char bankLetter;
            switch (mazeTile.ImageTileSet)
            {
                case 1:
                    bankLetter = 'a';
                    break;
                case 2:
                    bankLetter = 'b';
                    break;
                case 3:
                    bankLetter = 'c';
                    break;
                default:
                    bankLetter = 'a';
                    break;
            }

            this.itemIsPassable = mazeTile.IsPassable;

            this.itemOrigin = new Vector2(0, 0);
            this.itemTextureLocation = new Rectangle(((mazeTile.ImageNumber % 5) * 48) + 1, ((mazeTile.ImageNumber / 5) * 48) + 1, 47, 47);

            this.mazeItemTextures = new Texture2D[AnimationSteps];
            for (int tempAnimationState = 0; tempAnimationState < AnimationSteps; tempAnimationState++)
            {
                string fileName = String.Format(CultureInfo.InvariantCulture, "Stages\\{0}\\level{1}{2}{3}", resourceDirectory, levelNumber, bankLetter, (tempAnimationState + 1).ToString(CultureInfo.InvariantCulture));
                this.mazeItemTextures[tempAnimationState] = parentComponent.PlayGameContent.Load<Texture2D>(fileName);
            }

            this.ParentComponent = parentComponent;
        }

        /// <summary>
        /// Gets a value indicating whether the tile is passable;
        /// </summary>
        public bool IsPassable
        {
            get
            {
                return this.itemIsPassable;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the tile is destructable
        /// </summary>
        public bool Destructible
        {
            get
            {
                return this.itemIsDestructible;
            }
        }

        /// <summary>
        /// Updates the maze item
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        public virtual void Update(GameTime gameTime)
        {
            this.animationTime -= gameTime.ElapsedGameTime;
            if (this.animationTime.TotalSeconds <= 0)
            {
                this.animationState = (this.animationState + 1) % 4;
                this.animationTime = this.animationTime.Add(TimeSpan.FromMilliseconds(250));
            }
        }

        /// <summary>
        /// Draws the maze screen and everything in the maze.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time in the game</param>
        /// <param name="mazeSpriteBatch">The sprite batch being used to draw the maze.  This must have all graphics states set correctly</param>
        /// <param name="fade">The fading level of the maze, to allow it to fade in smoothly</param>
        public void Draw(GameTime gameTime, SpriteBatch mazeSpriteBatch, byte fade)
        {
            if (null != this.underMazeItem)
            {
                this.underMazeItem.Draw(gameTime, mazeSpriteBatch, fade);
            }

            Color gameColor = new Color(230, 230, 230);
            gameColor.R = Math.Min(gameColor.R, fade);
            gameColor.G = Math.Min(gameColor.G, fade);
            gameColor.B = Math.Min(gameColor.B, fade);
            mazeSpriteBatch.Draw(
                this.mazeItemTextures[this.animationState], this.itemPosition, this.itemTextureLocation, gameColor, 0.0f, this.itemOrigin, 1.0f, SpriteEffects.None, this.itemDepth);
        }

        /// <summary>
        /// Creates a new maze item
        /// </summary>
        /// <param name="parentComponent">The component that will be managing this maze item</param>
        /// <param name="resourceDirectory">The content manager directory in which the tiles can be found</param>
        /// <param name="levelNumber">The level number that we are creating this tile for</param>
        /// <param name="mazeTile">The maze tile object that is telling what sort of object to create</param>
        /// <param name="rowCounter">The row this item is to be placed at</param>
        /// <param name="columnCounter">The column this item is to be placed at</param>
        /// <returns>An MazeItem object representing the tile that was just created</returns>
        internal static MazeItem MakeMazeItem(MazeGameComponent parentComponent, string resourceDirectory, int levelNumber, MazeMap.MazeTile mazeTile, int rowCounter, int columnCounter)
        {
            if (mazeTile.Is3D)
            {
                return new ThreeDMazeItem(parentComponent, resourceDirectory, levelNumber, mazeTile, rowCounter, columnCounter);
            }
            else if (mazeTile.IsDestructable)
            {
                return new DestructableMazeItem(parentComponent, resourceDirectory, levelNumber, mazeTile, rowCounter, columnCounter);
            }
            else
            {
                return new MazeItem(parentComponent, resourceDirectory, levelNumber, mazeTile, rowCounter, columnCounter);
            }
        }
    }
}
