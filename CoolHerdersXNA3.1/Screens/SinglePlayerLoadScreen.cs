//-----------------------------------------------------------------------------
// <copyright file="ProfileSignInScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using CoolHerders.Housekeeping;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// </summary>
    internal class SinglePlayerLoadScreen : GameScreen
    {
        private bool haveInitiatedLoad;
        private bool haveShownMessage;
        private string storyModeFileName;
        private int continueLevel;

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the ProfileSignInScreen class
        /// </summary>
        /// <param name="sessionType">The network session type we eventually want to create</param>
        public SinglePlayerLoadScreen(string storyModeFileName)
        {
            this.storyModeFileName = storyModeFileName;
            IsPopup = true;
        }

        #endregion

        #region Update

        /// <summary>
        /// Updates the profile sign in screen.
        /// </summary>
        /// <param name="gameTime">The current GameTime of the game</param>
        /// <param name="otherScreenHasFocus">Does some other screen have focus</param>
        /// <param name="coveredByOtherScreen">Is this screen covered by another screen</param>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
            if (!haveInitiatedLoad)
            {
                ((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).LoadSavedGame();
                haveInitiatedLoad = true;
            }
            else
            {
                if (!((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).LoadRequested)
                {
                    this.continueLevel = ((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).SavedInfo.lastCompletedLevel;
                    if (this.continueLevel > 0)
                    {
                        if (!this.haveShownMessage)
                        {
                            MessageBoxScreen messageScreen = new MessageBoxScreen("A game is in progress, do you want to continue or start over?");
                            messageScreen.Accepted += new EventHandler<EventArgs>(messageScreen_Accepted);
                            messageScreen.Canceled += new EventHandler<EventArgs>(messageScreen_Canceled);
                            ScreenManager.AddScreen(messageScreen);
                            this.haveShownMessage = true;
                        }
                    }
                    else
                    {
                        this.StartStoryModeGame(this.storyModeFileName, 0);
                    }
                }
            }
        }

        void messageScreen_Accepted(object sender, EventArgs e)
        {
            this.StartStoryModeGame(this.storyModeFileName, this.continueLevel);
        }

        void messageScreen_Canceled(object sender, EventArgs e)
        {
            PlayerSavedInformation savedInfo = ((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).SavedInfo;
            savedInfo.lastCompletedLevel = 0;
            ((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).SavedInfo = savedInfo;
            ((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).SaveGame();
            this.StartStoryModeGame(this.storyModeFileName, 0);
        }

        private void StartStoryModeGame(string storyModeFileName, int continueLevel)
        {
            try
            {
                NetworkSession networkSession = CreateLocalNetworkSession(1, 1);
                networkSession.StartGame();
                GameInformation.Instance.StoryModeSequencer = new StoryMode.StoryModeSequencer(ScreenManager, networkSession, storyModeFileName, continueLevel);
                GameInformation.Instance.StoryModeSequencer.RunNextSequence(false, false);
            }
            catch (NetworkException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
            catch (GamerPrivilegeException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
        }

        private NetworkSession CreateLocalNetworkSession(int maxLocalGamers, int maxGamers)
        {
            // Begin an synchronous create network session operation.
            NetworkSession networkSession = NetworkSession.Create(
                NetworkSessionType.Local,
                maxLocalGamers,
                maxGamers);

            // Create a component that will manage the session we just created.
            NetworkSessionComponent.Create(ScreenManager, networkSession);

            return networkSession;
        }

        #endregion
    }
}
