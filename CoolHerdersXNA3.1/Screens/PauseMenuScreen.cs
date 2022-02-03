//-----------------------------------------------------------------------------
// <copyright file="PauseMenuScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    #region Using Statements
    using System;
    using CoolHerders.Housekeeping;
    using CoolHerders.Networking;
    using CoolHerders.Properties;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// The pause menu comes up over the top of the game,
    /// giving the player options to resume or quit.
    /// </summary>
    internal class PauseMenuScreen : MenuScreen
    {
        #region Fields

        /// <summary>
        /// The network session (if any) that we are playing as part of
        /// </summary>
        private NetworkSession networkSession;
        
        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the PauseMenuScreen class
        /// </summary>
        /// <param name="networkSession">The network session we are part of, if any</param>
        public PauseMenuScreen(NetworkSession networkSession)
            : base(Resources.Paused)
        {
            this.networkSession = networkSession;

            // Flag that there is no need for the game to transition
            // off when the pause menu is on top of it.
            IsPopup = true;

            // Add the Resume Game menu entry.
            MenuEntry resumeGameMenuEntry = new MenuEntry(Resources.ResumeGame);
            resumeGameMenuEntry.Selected += OnCancel;
            MenuEntries.Add(resumeGameMenuEntry);

            if (GameInformation.Instance.StoryModeSequencer != null)
            {
                // If this is a single player game, add the Quit menu entry.
                MenuEntry quitGameMenuEntry = new MenuEntry(Resources.QuitGame);
                quitGameMenuEntry.Selected += this.QuitGameMenuEntrySelected;
                MenuEntries.Add(quitGameMenuEntry);
            }
            else
            {
                // If we are hosting a network game, add the Return to Lobby menu entry.
                if (networkSession.IsHost)
                {
                    MenuEntry lobbyMenuEntry = new MenuEntry(Resources.ReturnToLobby);
                    lobbyMenuEntry.Selected += this.ReturnToLobbyMenuEntrySelected;
                    MenuEntries.Add(lobbyMenuEntry);
                }

                // Add the End/Leave Session menu entry.
                string leaveEntryText = networkSession.IsHost ? Resources.EndSession :
                                                                Resources.LeaveSession;

                MenuEntry leaveSessionMenuEntry = new MenuEntry(leaveEntryText);
                leaveSessionMenuEntry.Selected += this.LeaveSessionMenuEntrySelected;
                MenuEntries.Add(leaveSessionMenuEntry);
            }
        }

        #endregion

        #region Draw

        /// <summary>
        /// Draws the pause menu screen. This darkens down the gameplay screen
        /// that is underneath us, and then chains to the base MenuScreen.Draw.
        /// </summary>
        /// <param name="gameTime">The current GameTime of the game</param>
        public override void Draw(GameTime gameTime)
        {
            ScreenManager.FadeBackBufferToBlack(TransitionAlpha * 2 / 3);

            base.Draw(gameTime);
        }

        #endregion

        #region Handle Input

        /// <summary>
        /// Event handler for when the Quit Game menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void QuitGameMenuEntrySelected(object sender, EventArgs e)
        {
            MessageBoxScreen confirmQuitMessageBox =
                                    new MessageBoxScreen(Resources.ConfirmQuitGame);

            confirmQuitMessageBox.Accepted += this.ConfirmQuitMessageBoxAccepted;

            ScreenManager.AddScreen(confirmQuitMessageBox);
        }

        /// <summary>
        /// Event handler for when the user selects ok on the "are you sure
        /// you want to quit" message box. This uses the loading screen to
        /// transition from the game back to the main menu screen.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void ConfirmQuitMessageBoxAccepted(object sender, EventArgs e)
        {
            if (this.networkSession.SessionState == NetworkSessionState.Playing)
            {
                NetworkSessionComponent.LeaveSession(ScreenManager, true);
            }
            LoadingScreen.Load(ScreenManager, false, new BackgroundScreen(), new MainMenuScreen());
        }

        /// <summary>
        /// Event handler for when the Return to Lobby menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void ReturnToLobbyMenuEntrySelected(object sender, EventArgs e)
        {
            if (this.networkSession.SessionState == NetworkSessionState.Playing)
            {
                this.networkSession.EndGame();
            }
        }

        /// <summary>
        /// Event handler for when the End/Leave Session menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void LeaveSessionMenuEntrySelected(object sender, EventArgs e)
        {
            NetworkSessionComponent.LeaveSession(ScreenManager, false);
        }

        #endregion
    }
}
