//-----------------------------------------------------------------------------
// <copyright file="PickupItem.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Net;
    using Microsoft.Xna.Framework.GamerServices;

    /// <summary>
    /// An item located in the maze which can be picked up by a herder.
    /// </summary>
    internal abstract class PickupItem : ICollidable
    {
        /// <summary>
        /// The game component which is managing this item
        /// </summary>
        internal MazeGameComponent ParentComponent;

        /// <summary>
        /// The number of steps in the character's animation
        /// </summary>
        private const int AnimationSteps = 4;

        /// <summary>
        /// Can we collide with this sprite
        /// </summary>
        private bool checkCollisions = true;

        /// <summary>
        /// The bounding box that will be used for the collision
        /// </summary>
        private BoundingBox collisionBox;

        /// <summary>
        /// This tracks the time the animation has been running
        /// </summary>
        private TimeSpan animationTime = TimeSpan.Zero;

        /// <summary>
        /// The state of the pickup animation
        /// </summary>
        private int animationState;

        /// <summary>
        /// The texture page that holds the powerup items
        /// </summary>
        private Texture2D miscItemsTexture;

        /// <summary>
        /// The position of this pickup in screen coordinates
        /// </summary>
        private Vector2 pickupPosition;

        /// <summary>
        /// The depth of this pickup for pseudo-3d porpoises
        /// </summary>
        private float pickupDepth;

        /// <summary>
        /// The location of this pickup graphic on the texture page
        /// </summary>
        private Rectangle pickupTextureLocation;

        /// <summary>
        /// Which powerup graphic should be used for this powerup
        /// </summary>
        private int powerupGraphic;

        /// <summary>
        /// The offset to place on characters to make them lock to the maze grid correctly
        /// </summary>
        private Vector2 drawingOffset = new Vector2(16, 0);

        /// <summary>
        /// The network gamer that is controlling this item
        /// </summary>
        private NetworkGamer networkGamer;

        /// <summary>
        /// Initializes a new instance of the PickupItem class.
        /// </summary>
        /// <param name="parentComponent">The component which is running the game</param>
        /// <param name="networkGamer">The gamer that owns this object for network purposes</param>
        /// <param name="powerupGraphicNumber">Which graphic number should be used to represent this powerup</param>
        /// <param name="rowCounter">The row this pickup should appear on</param>
        /// <param name="columnCounter">The column this pickup should appear on</param>
        public PickupItem(MazeGameComponent parentComponent, NetworkGamer gamerInfo, int powerupGraphicNumber, int rowCounter, int columnCounter)
        {
            this.miscItemsTexture = parentComponent.PlayGameContent.Load<Texture2D>("MiscGraphics\\powerupsetc");
            this.pickupPosition = new Vector2(columnCounter * 32, rowCounter * 32);
            this.pickupDepth = ((rowCounter - 1) * 0.05f) + ((columnCounter + 1) * 0.002f) + 0.009f;
            this.collisionBox = new BoundingBox(new Vector3(this.pickupPosition.X, this.pickupPosition.Y, 0.0f), new Vector3(this.pickupPosition.X + 24, this.pickupPosition.Y + 16, 0.0f));
            this.powerupGraphic = powerupGraphicNumber;
            this.ParentComponent = parentComponent;
        }

        /// <summary>
        /// Gets or sets the collision box
        /// </summary>
        public BoundingBox CollisionBox
        {
            get { return this.collisionBox; }
            set { this.collisionBox = value; }
        }

        /// <summary>
        /// Gets or sets a value indicating whether collisions should be checked
        /// </summary>
        public bool CheckCollisions
        {
            get { return this.checkCollisions; }
            set { this.checkCollisions = value; }
        }

        /// <summary>
        /// Draws a pickup item
        /// </summary>
        /// <param name="mazeSpriteBatch">The sprite batch being used to draw the maze.  This must have all graphics states set correctly</param>
        /// <param name="transitionColor">The color number that should be applied to help cope with any transition effects</param>
        /// <param name="alphaChannel">An additional alpha channel to use when drawing the item</param>
        public virtual void Draw(SpriteBatch mazeSpriteBatch, byte transitionColor, byte alphaChannel)
        {
            mazeSpriteBatch.Draw(
                this.miscItemsTexture, this.pickupPosition + this.drawingOffset, this.pickupTextureLocation, new Color(transitionColor, transitionColor, transitionColor, alphaChannel), 0.0f, new Vector2(24.0f, 24.0f), 1.0f, SpriteEffects.None, this.pickupDepth);
        }

        /// <summary>
        /// Called when this powerup is picked up by the player
        /// </summary>
        /// <param name="acquiringCharacter">The character that picked us up</param>
        public abstract void OnPickup(MovingCharacter acquiringCharacter);

        /// <summary>
        /// Updates the player character
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        internal virtual void Update(GameTime gameTime)
        {
            this.animationTime -= gameTime.ElapsedGameTime;
            if (this.animationTime.Seconds <= 0)
            {
                this.animationState = (this.animationState + 1) % 4;
                this.animationTime = this.animationTime.Add(TimeSpan.FromMilliseconds(250));
            }

            this.pickupTextureLocation = new Rectangle(this.animationState * 48, (this.powerupGraphic * 48), 48, 48);
        }
    }
}
