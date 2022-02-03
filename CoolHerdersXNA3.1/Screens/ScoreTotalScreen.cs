//-----------------------------------------------------------------------------
// <copyright file="ScoreTotalScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using CoolHerders.Housekeeping;
    using CoolHerders.LevelDescriptions;
    using CoolHerders.Networking;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;

    /// <summary>
    /// A screen which is shown to total up the player's current score (in sheep captured so far)
    /// </summary>
    internal class ScoreTotalScreen : GameScreen
    {
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
        /// The origins for each player's scoring window
        /// </summary>
        private static readonly Vector2[] playerScoreWindow =
        {
          new Vector2(0, 40),
          new Vector2(0, 210),
          new Vector2(0, 380),
          new Vector2(0, 550)
        };

        /// <summary>
        /// The content manager to use for drawing per-session graphics, this should be copied from the MazeScreen for best performance
        /// </summary>
        private ContentManager contentManager;

        /// <summary>
        /// The texture which will appear behind each player's info
        /// </summary>
        private Texture2D playerBackgroundTexture;

        /// <summary>
        /// A texture for the white sheep
        /// </summary>
        private Texture2D whiteSheepTexture;

        /// <summary>
        /// Samples of the characters to display the AI
        /// </summary>
        private Texture2D characterSamples;

        /// <summary>
        /// The network session, if any, of which this game was a part
        /// </summary>
        private NetworkSession networkSession;

        /// <summary>
        /// The font used to draw the gamertag
        /// </summary>
        private SpriteFont gamerFont;

        /// <summary>
        /// This tracks the time till the next sheep is shown
        /// </summary>
        private TimeSpan addSheepTime = TimeSpan.Zero;

        /// <summary>
        /// The maximum sheep that should be drawn at the moment
        /// </summary>
        private int maximumSheepToDraw;

        /// <summary>
        /// The maximum sheep that we will have to draw for the strongest player
        /// </summary>
        private int maximumSheep;

        /// <summary>
        /// Initializes a new instance of the ScoreTotalScreen class
        /// </summary>
        /// <param name="networkSession">The network session that this game is running under</param>
        /// <param name="contentManager">The content manager of the maze screen, so that we may make use of it's cache</param>
        public ScoreTotalScreen(NetworkSession networkSession, ContentManager contentManager)
        {
            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);
            this.contentManager = contentManager;
            this.networkSession = networkSession;
            this.maximumSheepToDraw = 0;
            this.addSheepTime = TimeSpan.FromSeconds(1.5);
        }

        /// <summary>
        /// Loads all content for this screen, preferably from the maze screen's content manager to increase performance (it should be cached there)
        /// </summary>
        public override void LoadContent()
        {
            this.playerBackgroundTexture = this.contentManager.Load<Texture2D>("MiscGraphics\\gradient2");
            this.gamerFont = this.contentManager.Load<SpriteFont>("Arial");
            this.whiteSheepTexture = this.contentManager.Load<Texture2D>("Characters\\Sheep\\SheepWH");
            this.characterSamples = this.contentManager.Load<Texture2D>("Characters\\characters");
            this.maximumSheep = this.GetMaximumSheep();
        }

        /// <summary>
        /// Handles the input to this screen, for when to move on
        /// </summary>
        /// <param name="input">The input state of the controllers</param>
        public override void HandleInput(InputState input)
        {
            if (input.IsNewButtonPress(Buttons.A) && (this.maximumSheep == this.maximumSheepToDraw))
            {
                if (GameInformation.Instance.StoryModeSequencer != null)
                {
                    GameInformation.Instance.StoryModeSequencer.RunNextSequence(false, false);
                }
                else
                {
                    if (GameInformation.Instance.CurrentStageNumber == 2)
                    {
                        if (this.networkSession.IsHost && (this.networkSession.SessionState == NetworkSessionState.Playing))
                        {
                            this.networkSession.EndGame();
                        }
                    }
                    else
                    {
                        GameInformation.Instance.CurrentStageNumber++;
                        LoadingScreen.Load(
                            ScreenManager,
                            true,
                            new MazeScreen(this.networkSession));
                    }
                }
            }

            if (input.PauseGame)
            {
                ScreenManager.AddScreen(new PauseMenuScreen(this.networkSession));
            }
        }

        /// <summary>
        /// Updates this screen, including drawing more sheep if the timer has elapsed, as well as loading the next screen as appropriate
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        /// <param name="otherScreenHasFocus">Boolean set to true if some other screen has focus</param>
        /// <param name="coveredByOtherScreen">Boolean set to true if some other screen covers us</param>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            if ((this.networkSession != null) && !IsExiting)
            {
                if (this.networkSession.SessionState == NetworkSessionState.Lobby)
                {
                    LoadingScreen.Load(ScreenManager, true, new BackgroundScreen(), new LobbyScreen(this.networkSession));
                }
            }

            if (this.addSheepTime.Ticks > 0)
            {
                this.addSheepTime = this.addSheepTime.Subtract(gameTime.ElapsedGameTime);
                if (this.addSheepTime.Ticks <= 0)
                {
                    this.maximumSheepToDraw++;
                    ((CoolHerdersGame)ScreenManager.Game).PlayClick();
                    if (this.maximumSheepToDraw < this.maximumSheep)
                    {
                        this.addSheepTime = this.addSheepTime.Add(TimeSpan.FromSeconds(0.20));
                    }
                    else
                    {
                        this.addSheepTime = TimeSpan.Zero;
                    }
                }
            }
        }

        /// <summary>
        /// Draws all content on this screen
        /// </summary>
        /// <param name="gameTime">The current game time</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;

            spriteBatch.Begin();

            string scoreInfoString = "Results of the previous round...";
            Vector2 fontOrigin = this.gamerFont.MeasureString(scoreInfoString) / 2;

            spriteBatch.Draw(this.playerBackgroundTexture, new Rectangle(0, 0, 1280, 40), Color.Black);
            spriteBatch.DrawString(
                this.gamerFont, 
                scoreInfoString, 
                new Vector2(640, 20), 
                new Color(TransitionAlpha, TransitionAlpha, TransitionAlpha),
                0.0f, 
                fontOrigin, 
                1.0f, 
                SpriteEffects.None, 
                0.0f);
            this.DrawBackground(spriteBatch);

            if (this.networkSession != null)
            {
                foreach (NetworkGamer gamerInfo in this.networkSession.AllGamers)
                {
                    PlayerInformation playerInfo = (PlayerInformation)gamerInfo.Tag;
                    this.DrawPlayerInfo(spriteBatch, playerInfo, gamerInfo);
                }
            }

            if (GameInformation.Instance.EnemyPlayers != null)
            {
                foreach (PlayerInformation playerInfo in GameInformation.Instance.EnemyPlayers)
                {
                    this.DrawPlayerInfo(spriteBatch, playerInfo, null);
                }
            }

            spriteBatch.End();
        }

        /// <summary>
        /// Draws all info about the player
        /// </summary>
        /// <param name="spriteBatch">The sprite batch to use</param>
        /// <param name="playerInfo">The info about the player</param>
        /// <param name="gamerInfo">The gamer info about the player if this is a network game</param>
        private void DrawPlayerInfo(SpriteBatch spriteBatch, PlayerInformation playerInfo, NetworkGamer gamerInfo)
        {
            int playerSeatNumber = playerInfo.SeatNumber;
            Vector2 position = playerScoreWindow[playerSeatNumber];
            Vector2 gamerTagPosition = position + new Vector2(15, 100);
            if (gamerInfo != null)
            {
                spriteBatch.DrawString(this.gamerFont, gamerInfo.Gamertag, gamerTagPosition, new Color(TransitionAlpha, TransitionAlpha, TransitionAlpha));
            }
            else if (playerInfo.SignedInGamer != null)
            {
                spriteBatch.DrawString(this.gamerFont, playerInfo.SignedInGamer.Gamertag, gamerTagPosition, new Color(TransitionAlpha, TransitionAlpha, TransitionAlpha));
            }
            else
            {
                spriteBatch.DrawString(this.gamerFont, playerInfo.CharacterClass, gamerTagPosition, new Color(TransitionAlpha, TransitionAlpha, TransitionAlpha));
            }

            if (playerInfo.GamerProfile != null)
            {
                // XNA4.0 - GamerPicture removed - generate dynamically?
                Texture2D gamerPic = Texture2D.FromStream(spriteBatch.GraphicsDevice, playerInfo.GamerProfile.GetGamerPicture());
                if (null != gamerPic)
                {
                    Vector2 gamerPicturePosition = position + new Vector2(15, 10);
                    spriteBatch.Draw(gamerPic, gamerPicturePosition, new Color(TransitionAlpha, TransitionAlpha, TransitionAlpha));
                }
            }

            Vector2 sheepPosition = position + new Vector2(200, 10);
            float sheepDepth = 1.0f;
            for (int sheepCounter = 0; (sheepCounter < playerInfo.SheepCurrentRound) && (sheepCounter < this.maximumSheepToDraw); sheepCounter++)
            {
                spriteBatch.Draw(this.whiteSheepTexture, sheepPosition, new Rectangle(0, 192, 48, 48), new Color(TransitionAlpha, TransitionAlpha, TransitionAlpha), 0.0f, Vector2.Zero, 1.5f, SpriteEffects.None, sheepDepth);
                sheepPosition.X += 25;
                if (sheepPosition.X > 1200)
                {
                    sheepPosition.Y += 25;
                    sheepPosition.X = 200;
                    sheepDepth -= 0.1f;
                }
            }

            for (int sheepCounter = playerInfo.SheepCurrentRound; (sheepCounter < playerInfo.SheepTotal) && (sheepCounter < this.maximumSheepToDraw); sheepCounter++)
            {
                spriteBatch.Draw(this.whiteSheepTexture, sheepPosition, new Rectangle(0, 192, 48, 48), new Color(TransitionAlpha, TransitionAlpha, TransitionAlpha, 128), 0.0f, Vector2.Zero, 1.5f, SpriteEffects.None, sheepDepth);
                sheepPosition.X += 25;
                if (sheepPosition.X > 1200)
                {
                    sheepPosition.Y += 25;
                    sheepPosition.X = 200;
                    sheepDepth -= 0.1f;
                }
            }
        }

        /// <summary>
        /// Draws the backgrounds for all the players
        /// </summary>
        /// <param name="spriteBatch">The spritebatch to use</param>
        private void DrawBackground(SpriteBatch spriteBatch)
        {
            for (int playerSeatNumber = 0; playerSeatNumber < 4; playerSeatNumber++)
            {
                Vector2 position = playerScoreWindow[playerSeatNumber];
                Rectangle backgroundBox = new Rectangle((int)position.X, (int)position.Y, 1280, 170);
                spriteBatch.Draw(this.playerBackgroundTexture, backgroundBox, playerColors[playerSeatNumber]);
            }
        }

        /// <summary>
        /// Finds the player with the largest number of sheep
        /// </summary>
        /// <returns>Number of sheep</returns>
        private int GetMaximumSheep()
        {
            int biggestSheep = 0;

            if (this.networkSession != null)
            {
                foreach (NetworkGamer gamerInfo in this.networkSession.AllGamers)
                {
                    PlayerInformation playerInfo = (PlayerInformation)gamerInfo.Tag;
                    if (playerInfo.SheepTotal > biggestSheep)
                    {
                        biggestSheep = playerInfo.SheepTotal;
                    }
                }
            }

            if (GameInformation.Instance.EnemyPlayers != null)
            {
                foreach (PlayerInformation playerInfo in GameInformation.Instance.EnemyPlayers)
                {
                    if (playerInfo.SheepTotal > biggestSheep)
                    {
                        biggestSheep = playerInfo.SheepTotal;
                    }
                }
            }

            return biggestSheep;
        }
    }
}
