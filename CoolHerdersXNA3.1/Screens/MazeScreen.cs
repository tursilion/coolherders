//-----------------------------------------------------------------------------
// <copyright file="MazeScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    #region Using Statements
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Threading;
    using CoolHerders.GameItems;
    using CoolHerders.Housekeeping;
    using CoolHerders.LevelDescriptions;
    using CoolHerders.Networking;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// The background screen sits behind all the other menu screens.
    /// It draws a background image that remains fixed in place regardless
    /// of whatever transitions the screens on top of it may be doing.
    /// </summary>
    internal class MazeScreen : GameScreen, IDisposable
    {
        #region Fields

        /// <summary>
        /// Holds a list of additional players to work with
        /// </summary>
        public List<PlayerInformation> AdditionalPlayers;

        /// <summary>
        /// This holds the temporary content manager we will use for the screen
        /// </summary>
        private ContentManager content;

        /// <summary>
        /// Tracks if the game screen is still loading
        /// </summary>
        private bool isLoading;

        /// <summary>
        /// The maze component that handles most of our game logic
        /// </summary>
        private MazeGameComponent mazeComponent;

        /// <summary>
        /// The scoring component that draws the scores and gamertags
        /// </summary>
        private PlayerScoreComponent playerScores;

        /// <summary>
        /// The network session we are connected to if network multiplayer
        /// null if not connected to the network
        /// </summary>
        private NetworkSession networkSession;

        /// <summary>
        /// Holds the description of the current level
        /// </summary>
        private LevelDescription currentLevel;

        /// <summary>
        /// Holds a value indicating if we are playing the end of round jingle
        /// </summary>
        private bool playingEndingJingle;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the MazeScreen class
        /// </summary>
        /// <param name="networkSession">The network session which this maze screen is running under, or null if single player</param>
        public MazeScreen(NetworkSession networkSession)
        {
            if (networkSession == null)
            {
                throw new ArgumentNullException("networkSession", "Network session must now be valid");
            }

            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);

            this.networkSession = networkSession;
            this.currentLevel = GameInformation.Instance.WorldScreen;
            this.AdditionalPlayers = new List<PlayerInformation>(GameInformation.Instance.EnemyPlayers);
        }

        /// <summary>
        /// Finalizes an instance of the MazeScreen class
        /// </summary>
        ~MazeScreen()
        {
            this.Dispose(false);
        }

        /// <summary>
        /// Event raised when the game has finished.
        /// </summary>
        public event EventHandler<EventArgs> GameHasFinished;

        /// <summary>
        /// Gets a value indicating whether this screen is still loading
        /// </summary>
        public bool IsStillLoading
        {
            get
            {
                return this.isLoading;
            }
        }

        /// <summary>
        /// Gets the content manager used in this screen
        /// </summary>
        public ContentManager Content
        {
            get
            {
                return this.content;
            }
        }

        /// <summary>
        /// Gets the level description being used on this screen
        /// </summary>
        public LevelDescription CurrentLevel
        {
            get
            {
                return this.currentLevel;
            }
        }

        /// <summary>
        /// Gets the network session that is being run, if any
        /// </summary>
        public NetworkSession NetSession
        {
            get
            {
                return this.networkSession;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the screen is active.
        /// </summary>
        /// <remarks>
        /// The logic for deciding whether the game is paused depends on whether
        /// this is a networked or single player game. If we are in a network session,
        /// we should go on updating the game even when the user tabs away from us or
        /// brings up the pause menu, because even though the local player is not
        /// responding to input, other remote players may not be paused. In single
        /// player modes, however, we want everything to pause if the game loses focus.
        /// </remarks>
        public new bool IsActive
        {
            get
            {
                if (GameInformation.Instance.StoryModeSequencer != null)
                {
                    // Pause behavior for single player games.
                    return base.IsActive;
                }
                else
                {
                    // Pause behavior for networked games.
                    return !IsExiting;
                }
            }
        }

        /// <summary>
        /// Loads graphics content for this screen. The background texture is quite
        /// big, so we use our own local ContentManager to load it. This allows us
        /// to unload before going from the menus into the game itself, wheras if we
        /// used the shared ContentManager provided by the Game class, the content
        /// would remain loaded forever.
        /// </summary>
        public override void LoadContent()
        {
            if (this.content == null)
            {
                this.content = new ContentManager(ScreenManager.Game.Services, "Content");
            }

            if (null == this.mazeComponent)
            {
                this.mazeComponent = new MazeGameComponent(ScreenManager.Game, this);
                this.mazeComponent.DrawOrder = 1;
                ((CoolHerdersGame)ScreenManager.Game).Components.Add(this.mazeComponent);
            }

            if (null == this.playerScores)
            {
                this.playerScores = new PlayerScoreComponent(ScreenManager.Game, this, this.networkSession);
                this.playerScores.DrawOrder = 2;
                ((CoolHerdersGame)ScreenManager.Game).Components.Add(this.playerScores);
            }

            SignedInGamer.SignedOut += new EventHandler<SignedOutEventArgs>(this.SignedInGamer_SignedOut);

            this.isLoading = true;

            Thread.Sleep(1000);
        }

        /// <summary>
        /// Called when the input needs to be processed
        /// </summary>
        /// <param name="input">The input state for all controllers</param>
        public override void HandleInput(InputState input)
        {
            base.HandleInput(input);
            if (input.IsNewButtonPress(Buttons.LeftShoulder))
            {
                for (int rowCounter = 0; rowCounter < 15; rowCounter++)
                {
                    for (int columnCounter = 0; columnCounter < 21; columnCounter++)
                    {
                        MazeItem item = this.mazeComponent.GetMazeItem(rowCounter, columnCounter);
                        if (item.Destructible)
                        {
                            ((DestructableMazeItem)item).StartDestruction();
                        }
                    }
                }
            }

            if (input.PauseGame)
            {
                ScreenManager.AddScreen(new PauseMenuScreen(this.networkSession));
            }
        }

        /// <summary>
        /// Unloads graphics content for this screen.
        /// </summary>
        public override void UnloadContent()
        {
            SignedInGamer.SignedOut -= new EventHandler<SignedOutEventArgs>(this.SignedInGamer_SignedOut);
            ((CoolHerdersGame)ScreenManager.Game).Components.Remove(this.mazeComponent);
            ScreenManager.Game.Components.Remove(this.playerScores);
            this.mazeComponent.Dispose();
            this.playerScores.Dispose();
            this.mazeComponent = null;
            this.playerScores = null;
            ((CoolHerdersGame)ScreenManager.Game).PlayBackgroundAudioCue("TitleBackground");
        }

        #endregion

        #region Update and Draw

        /// <summary>
        /// Updates the maze screen screen.
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        /// <param name="otherScreenHasFocus">Boolean set to true if some other screen has focus</param>
        /// <param name="coveredByOtherScreen">Boolean set to true if some other screen covers us</param>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            if (this.IsActive)
            {
                if (this.isLoading)
                {
                    ((CoolHerdersGame)ScreenManager.Game).PlayBackgroundAudioCue(this.currentLevel.BackgroundSongName);
                    this.isLoading = false;
                }

                if (this.mazeComponent.FreeSheep == 0)
                {
                    if (this.playingEndingJingle)
                    {
                        if (!((CoolHerdersGame)ScreenManager.Game).IsBackgroundAudioCuePlaying())
                        {
                            LoadingScreen.Load(this.ScreenManager, false, new ScoreTotalScreen(this.networkSession, this.content));
                            this.content = null;
                        }
                    }
                    else
                    {
                        this.GameHasFinished(this, new EventArgs());
                        ((CoolHerdersGame)ScreenManager.Game).PlayBackgroundAudioCue("WinJingle");
                        this.playingEndingJingle = true;

                        foreach (NetworkGamer gamer in this.networkSession.AllGamers)
                        {
                            PlayerInformation playerInfo = (PlayerInformation)gamer.Tag;
                            int seatNumber = playerInfo.SeatNumber;
                            playerInfo.SheepCurrentRound = this.mazeComponent.GetNumberOfSheep(seatNumber);
                            playerInfo.SheepTotal += playerInfo.SheepCurrentRound;
                        }

                        foreach (PlayerInformation playerInfo in this.AdditionalPlayers)
                        {
                            int seatNumber = playerInfo.SeatNumber;
                            playerInfo.SheepCurrentRound = this.mazeComponent.GetNumberOfSheep(seatNumber);
                            playerInfo.SheepTotal += playerInfo.SheepCurrentRound;
                        }
                    }
                }

                foreach (NetworkGamer gamer in this.networkSession.AllGamers)
                {
                    int seatNumber = ((PlayerInformation)gamer.Tag).SeatNumber;
                    this.playerScores.SetNumberOfSheep(seatNumber, this.mazeComponent.GetNumberOfSheep(seatNumber));
                    this.playerScores.SetZapperStrength(seatNumber, this.mazeComponent.PlayerCharacters[seatNumber].PlayerZapper.ZapperPower);
                }

                foreach (PlayerInformation playerInfo in this.AdditionalPlayers)
                {
                    int seatNumber = playerInfo.SeatNumber;
                    this.playerScores.SetNumberOfSheep(seatNumber, this.mazeComponent.GetNumberOfSheep(seatNumber));
                    this.playerScores.SetZapperStrength(seatNumber, this.mazeComponent.PlayerCharacters[seatNumber].PlayerZapper.ZapperPower);
                }
            }

            // If we are in a network game, check if we should return to the lobby.
            if ((this.networkSession != null) && !IsExiting)
            {
                if (this.networkSession.SessionState == NetworkSessionState.Lobby)
                {
                    LoadingScreen.Load(ScreenManager, true, new BackgroundScreen(), new LobbyScreen(this.networkSession));
                }
            }
        }

        /// <summary>
        /// Draws the maze screen and everything in the maze.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time in the game</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;

            // XNA4.0
            //spriteBatch.Begin(SpriteBlendMode.AlphaBlend);
            spriteBatch.Begin();
            spriteBatch.End();
        }

        #endregion

        #region IDisposable Members

        /// <summary>
        /// This is the public dispose method that disposes managed and any unmanaged resources
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// This is the internal dispose method that disposes the resources it has been instructed to
        /// </summary>
        /// <param name="disposing">Set to true if it is ok to dispose any managed resources</param>
        private void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.content != null)
                {
                    this.content.Dispose();
                    this.content = null;
                }

                this.mazeComponent.Dispose();
                this.playerScores.Dispose();
            }
        }
        #endregion

        /// <summary>
        /// Invoked when the message screen is cancelled
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void MessageScreen_Canceled(object sender, EventArgs e)
        {
            NetworkSessionComponent.LeaveSession(ScreenManager, true);
            LoadingScreen.Load(ScreenManager, false, new BackgroundScreen(), new MainMenuScreen());
        }

        /// <summary>
        /// Invoked when the message screen is accepted
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void MessageScreen_Accepted(object sender, EventArgs e)
        {
            NetworkSessionComponent.LeaveSession(ScreenManager, true);
            LoadingScreen.Load(ScreenManager, false, new BackgroundScreen(), new MainMenuScreen());
        }

        /// <summary>
        /// Invoked when a signed in player suddenly signs out
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="e">Signed Out event args</param>
        private void SignedInGamer_SignedOut(object sender, SignedOutEventArgs e)
        {
            MessageBoxScreen messageScreen = new MessageBoxScreen("A player has signed out.  Returning to main menu.");
            messageScreen.Accepted += new EventHandler<EventArgs>(this.MessageScreen_Accepted);
            messageScreen.Canceled += new EventHandler<EventArgs>(this.MessageScreen_Canceled);
            ScreenManager.AddScreen(messageScreen);
        }
    }
}
