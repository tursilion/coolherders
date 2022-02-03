using System;
using System.Collections.Generic;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Audio;
using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.GamerServices;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Microsoft.Xna.Framework.Media;
using Microsoft.Xna.Framework.Net;
using Microsoft.Xna.Framework.Storage;


namespace CoolHerders.GameComponents
{
    /// <summary>
    /// This is a game component that implements IUpdateable.
    /// </summary>
    public class SafeAreasComponent : Microsoft.Xna.Framework.DrawableGameComponent, CoolHerders.GameComponents.ISafeAreasComponent
    {
        Rectangle titleSafeArea;
        Rectangle titleUnsafeAreaLeft;
        Rectangle titleUnsafeAreaRight;
        Rectangle titleUnsafeAreaTop;
        Rectangle titleUnsafeAreaBottom;
        Game ourGame;
        Texture2D titleUnsafeTexture;
        SpriteBatch spriteBatch;
        bool drawUnsafeBorder;

        public SafeAreasComponent(Game game)
            : base(game)
        {
            this.ourGame = game;
        }

        public bool DrawUnsafeBorder
        {
            get
            {
                return this.drawUnsafeBorder;
            }
            set
            {
                this.drawUnsafeBorder = value;
            }
        }

        /// <summary>
        /// Allows the game component to perform any initialization it needs to before starting
        /// to run.  This is where it can query for any required services and load content.
        /// </summary>
        public override void Initialize()
        {
            Viewport ourViewport = this.ourGame.GraphicsDevice.Viewport;
            this.titleSafeArea = ourViewport.TitleSafeArea;
/*
            int unsafeWidth = (int)(ourViewport.Width * 0.20);
            int unsafeHeight = (int)(ourViewport.Height * 0.20);
            this.titleSafeArea = new Rectangle(ourViewport.X + (unsafeWidth / 2), ourViewport.Y + (unsafeHeight / 2), ourViewport.Width - unsafeWidth, ourViewport.Height - unsafeHeight);
*/

            this.titleUnsafeAreaLeft = new Rectangle(ourViewport.X, ourViewport.Y, this.titleSafeArea.Left - ourViewport.X, ourViewport.Height);
            this.titleUnsafeAreaRight = new Rectangle(this.titleSafeArea.Right, ourViewport.Y, ourViewport.Width - this.titleSafeArea.Right, ourViewport.Height);
            this.titleUnsafeAreaTop = new Rectangle(this.titleSafeArea.Left, ourViewport.Y, this.titleSafeArea.Width, this.titleSafeArea.Top - ourViewport.Y);
            this.titleUnsafeAreaBottom = new Rectangle(this.titleSafeArea.Left, this.titleSafeArea.Bottom, this.titleSafeArea.Width, ourViewport.Height - this.titleSafeArea.Bottom);

            this.ourGame.Services.AddService(typeof(ISafeAreasComponent), this);

            base.Initialize();
        }

        protected override void LoadContent()
        {
            this.titleUnsafeTexture = new Texture2D(this.ourGame.GraphicsDevice, 10, 10);
            Color[] fillColors = new Color[10 * 10];
            for (int counter = 0; counter < fillColors.Length; counter++)
            {
                // XNA4.0 - Color constructor no longer takes a color object
                fillColors[counter] = new Color(Color.Red.R, Color.Red.G, Color.Red.B, 128);
            }
            this.titleUnsafeTexture.SetData(fillColors);
            this.spriteBatch = new SpriteBatch(this.ourGame.GraphicsDevice);

            base.LoadContent();
        }

        /// <summary>
        /// Allows the game component to update itself.
        /// </summary>
        /// <param name="gameTime">Provides a snapshot of timing values.</param>
        public override void Update(GameTime gameTime)
        {
            // TODO: Add your update code here

            base.Update(gameTime);
        }

        public override void Draw(GameTime gameTime)
        {
            if (this.drawUnsafeBorder)
            {
                this.spriteBatch.Begin();
                this.spriteBatch.Draw(this.titleUnsafeTexture, this.titleUnsafeAreaTop, Color.White);
                this.spriteBatch.Draw(this.titleUnsafeTexture, this.titleUnsafeAreaBottom, Color.White);
                this.spriteBatch.Draw(this.titleUnsafeTexture, this.titleUnsafeAreaLeft, Color.White);
                this.spriteBatch.Draw(this.titleUnsafeTexture, this.titleUnsafeAreaRight, Color.White);
                this.spriteBatch.End();
            }
            base.Draw(gameTime);
        }
    }
}