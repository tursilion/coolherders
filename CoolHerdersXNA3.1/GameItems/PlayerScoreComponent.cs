//-----------------------------------------------------------------------------
// <copyright file="PlayerScoreComponent.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Audio;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;
    using Microsoft.Xna.Framework.Storage;

    /// <summary>
    /// This is a game component that implements IUpdateable.
    /// </summary>
    public class PlayerScoreComponent : Microsoft.Xna.Framework.DrawableGameComponent
    {
        /// <summary>
        /// The rectangles to draw for each player's score background
        /// </summary>
        private static readonly Rectangle[] playerBackgrounds =
        {
          new Rectangle(1000, 0, 280, 180),
          new Rectangle(1000, 180, 280, 180),
          new Rectangle(1000, 360, 280, 180),
          new Rectangle(1000, 540, 280, 180)
        };

        /// <summary>
        /// The screen coordinates to draw each player's gamer tag at
        /// </summary>
        private static readonly Vector2[] playerGamerTags =
        {
          new Vector2(1020, 50),
          new Vector2(1020, 230),
          new Vector2(1020, 410),
          new Vector2(1020, 590)
        };

        /// <summary>
        /// The screen coordinates to draw each player's image at
        /// </summary>
        private static readonly Vector2[] playerGamerImages =
        {
          new Vector2(1048, 100),
          new Vector2(1048, 280),
          new Vector2(1048, 460),
          new Vector2(1048, 640)
        };

        /// <summary>
        /// The screen coordinates to draw each player's score at
        /// </summary>
        private static readonly Vector2[] playerScoreLocation = 
        {
            new Vector2(1150, 80),
            new Vector2(1150, 260),
            new Vector2(1150, 440),
            new Vector2(1150, 620),
        };

        /// <summary>
        /// The screen coordinates to draw each player's zapper strength at
        /// </summary>
        private static readonly Vector2[] playerZapperStrengthLocation = 
        {
            new Vector2(1150, 130),
            new Vector2(1150, 310),
            new Vector2(1150, 490),
            new Vector2(1150, 670),
        };

        /// <summary>
        /// The names to display if the player is not signed in
        /// </summary>
        private static readonly string[] playerDefaultNames =
        {
          "Player 1",
          "Player 2",
          "Player 3",
          "Player 4",
        };

        /// <summary>
        /// The colors to tint each player's background
        /// </summary>
        private static readonly Color[] playerColors =
        {
            Color.LightBlue,
            Color.LightGreen,
            Color.LightYellow,
            Color.LightPink,
        };

        /// <summary>
        /// The content manager into which we will load our resources
        /// </summary>
        private ContentManager contentManager;

        /// <summary>
        /// The maze screen which we are part of
        /// </summary>
        private MazeScreen mazeScreen;

        /// <summary>
        /// The sprite batch used to draw all the players
        /// </summary>
        private SpriteBatch playerSpriteBatch;

        /// <summary>
        /// The background texture to put behind each player's information
        /// </summary>
        private Texture2D backgroundTexture;

        /// <summary>
        /// The game object which we are running under
        /// </summary>
        private CoolHerdersGame theGame;

        /// <summary>
        /// The font to use to draw gamertags with
        /// </summary>
        private SpriteFont gamerFont;

        /// <summary>
        /// Number of sheep held by the given player
        /// </summary>
        private int[] numberOfSheep = new int[4];

        /// <summary>
        /// Zapper strength of the given player
        /// </summary>
        private int[] zapperStrength = new int[4];

        /// <summary>
        /// The network session we are part of
        /// </summary>
        private NetworkSession networkSession;

        /// <summary>
        /// Initializes a new instance of the PlayerScoreComponent class
        /// </summary>
        /// <param name="game">The game object we are running under</param>
        /// <param name="mazeScreen">The maze screen object we are part of</param>
        /// <param name="networkSession">The network session we are part of</param>
        internal PlayerScoreComponent(Game game, MazeScreen mazeScreen, NetworkSession networkSession)
            : base(game)
        {
            this.mazeScreen = mazeScreen;
            this.contentManager = mazeScreen.Content;
            this.theGame = (CoolHerdersGame)game;
            this.networkSession = networkSession;
        }

        /// <summary>
        /// Allows the game component to perform any initialization it needs to before starting
        /// to run.  This is where it can query for any required services and load content.
        /// </summary>
        public override void Initialize()
        {
            base.Initialize();
        }

        /// <summary>
        /// Allows the game component to update itself.
        /// </summary>
        /// <param name="gameTime">Provides a snapshot of timing values.</param>
        public override void Update(GameTime gameTime)
        {
            //// TODO: Add your update code here

            base.Update(gameTime);
        }

        /// <summary>
        /// Draws the player scores to the screen
        /// </summary>
        /// <param name="gameTime">The current GameTime of the game</param>
        public override void Draw(GameTime gameTime)
        {
            byte fade = this.mazeScreen.TransitionAlpha;
            string gamerString;

            base.Draw(gameTime);
            this.playerSpriteBatch.Begin();

            for (PlayerIndex playerIndex = PlayerIndex.One; playerIndex <= PlayerIndex.Four; playerIndex++)
            {
                this.playerSpriteBatch.Draw(this.backgroundTexture, playerBackgrounds[(int)playerIndex], playerColors[(int)playerIndex]);
            }

            if (this.networkSession != null)
            {
                foreach (NetworkGamer gamer in this.networkSession.AllGamers)
                {
                    PlayerInformation playerInfo = ((PlayerInformation)gamer.Tag);

                    if (playerInfo.PlayerActive)
                    {
                        gamerString = gamer.Gamertag;

                        this.playerSpriteBatch.DrawString(this.gamerFont, gamerString, playerGamerTags[playerInfo.SeatNumber], Color.White);

                        if (playerInfo.GamerProfile != null)
                        {
                            // XNA4.0 - GamerPicture removed - generate dynamically?
                            Texture2D gamerPic = Texture2D.FromStream(this.GraphicsDevice, playerInfo.GamerProfile.GetGamerPicture());
                            if (null != gamerPic)
                            {
                                this.playerSpriteBatch.Draw(gamerPic, playerGamerImages[playerInfo.SeatNumber], new Color(fade, fade, fade));
                            }
                        }

                        this.playerSpriteBatch.DrawString(this.gamerFont, this.numberOfSheep[playerInfo.SeatNumber].ToString(CultureInfo.CurrentCulture), playerScoreLocation[playerInfo.SeatNumber], Color.White);
                        this.playerSpriteBatch.DrawString(this.gamerFont, this.zapperStrength[playerInfo.SeatNumber].ToString(CultureInfo.CurrentCulture), playerZapperStrengthLocation[playerInfo.SeatNumber], Color.White);
                    }
                }
            }

            foreach (PlayerInformation playerInfo in this.mazeScreen.AdditionalPlayers)
            {
                if (playerInfo.PlayerActive)
                {
                    if (playerInfo.SignedInGamer != null)
                    {
                        gamerString = playerInfo.SignedInGamer.Gamertag;
                    }
                    else
                    {
                        gamerString = playerInfo.CharacterClass;
                    }

                    this.playerSpriteBatch.DrawString(this.gamerFont, gamerString, playerGamerTags[playerInfo.SeatNumber], Color.White);

                    if (playerInfo.GamerProfile != null)
                    {
                        // XNA4.0 - GamerPicture removed - generate dynamically?
                        Texture2D gamerPic = Texture2D.FromStream(this.GraphicsDevice, playerInfo.GamerProfile.GetGamerPicture());
                        if (null != gamerPic)
                        {
                            this.playerSpriteBatch.Draw(gamerPic, playerGamerImages[playerInfo.SeatNumber], new Color(fade, fade, fade));
                        }
                    }

                    this.playerSpriteBatch.DrawString(this.gamerFont, this.numberOfSheep[playerInfo.SeatNumber].ToString(CultureInfo.CurrentCulture), playerScoreLocation[playerInfo.SeatNumber], Color.White);
                    this.playerSpriteBatch.DrawString(this.gamerFont, this.zapperStrength[playerInfo.SeatNumber].ToString(CultureInfo.CurrentCulture), playerZapperStrengthLocation[playerInfo.SeatNumber], Color.White);
                }
            }

            this.playerSpriteBatch.End();
        }

        /// <summary>
        /// Sets the number of sheep that a player is holding
        /// </summary>
        /// <param name="playerSeatNumber">The player seat number to update</param>
        /// <param name="numberOfSheep">The number of sheep that player now has</param>
        public void SetNumberOfSheep(int playerSeatNumber, int numberOfSheep)
        {
            this.numberOfSheep[playerSeatNumber] = numberOfSheep;
        }

        /// <summary>
        /// Sets the strength of the player's zapper
        /// </summary>
        /// <param name="playerSeatNumber">The player seat number to update</param>
        /// <param name="zapperStrength">The current power of the player's zapper</param>
        public void SetZapperStrength(int playerSeatNumber, int zapperStrength)
        {
            this.zapperStrength[playerSeatNumber] = zapperStrength;
        }

        /// <summary>
        /// Loads the graphics needed to display the gamer info
        /// </summary>
        protected override void LoadContent()
        {
            base.LoadContent();

            this.playerSpriteBatch = new SpriteBatch(this.theGame.GraphicsDevice);
            this.backgroundTexture = this.contentManager.Load<Texture2D>("MiscGraphics\\gradient2");
            this.gamerFont = this.contentManager.Load<SpriteFont>("Arial");
        }
    }
}