//-----------------------------------------------------------------------------
// <copyright file="JoinSessionScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using CoolHerders.Properties;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// This menu screen displays a list of available network sessions,
    /// and lets the user choose which one to join.
    /// </summary>
    internal class JoinSessionScreen : MenuScreen
    {
        #region Fields

        /// <summary>
        /// The maximum number of sessions that will be found in a search
        /// </summary>
        private const int MaxSearchResults = 8;

        /// <summary>
        /// The list of sessions that are available
        /// </summary>
        private AvailableNetworkSessionCollection availableSessions;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the JoinSessionScreen class.
        /// </summary>
        /// <param name="availableSessions">The list of available network sessions</param>
        public JoinSessionScreen(AvailableNetworkSessionCollection availableSessions)
            : base(Resources.JoinSession)
        {
            this.availableSessions = availableSessions;

            foreach (AvailableNetworkSession availableSession in availableSessions)
            {
                // Create menu entries for each available session.
                MenuEntry menuEntry = new AvailableSessionMenuEntry(availableSession);
                menuEntry.Selected += this.AvailableSessionMenuEntrySelected;
                MenuEntries.Add(menuEntry);

                // Matchmaking can return up to 25 available sessions at a time, but
                // we don't have room to fit that many on the screen. In a perfect
                // world we should make the menu scroll if there are too many, but it
                // is easier to just not bother displaying more than we have room for.
                if (MenuEntries.Count >= MaxSearchResults)
                {
                    break;
                }
            }

            // Add the Back menu entry.
            MenuEntry backMenuEntry = new MenuEntry(Resources.Back);
            backMenuEntry.Selected += this.BackMenuEntrySelected;
            MenuEntries.Add(backMenuEntry);
        }

        #endregion
           
        #region Event Handlers

        /// <summary>
        /// Event handler for when an available session menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void AvailableSessionMenuEntrySelected(object sender, EventArgs e)
        {
            // Which menu entry was selected?
            AvailableSessionMenuEntry menuEntry = (AvailableSessionMenuEntry)sender;
            AvailableNetworkSession availableSession = menuEntry.AvailableSession;

            try
            {
                // Begin an asynchronous join network session operation.
                IAsyncResult asyncResult = NetworkSession.BeginJoin(
                    availableSession,
                    null,
                    null);

                // Activate the network busy screen, which will display
                // an animation until this operation has completed.
                NetworkBusyScreen busyScreen = new NetworkBusyScreen(asyncResult);

                busyScreen.OperationCompleted += this.JoinSessionOperationCompleted;

                ScreenManager.AddScreen(busyScreen);
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

        /// <summary>
        /// Event handler for when the asynchronous join network session
        /// operation has completed.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void JoinSessionOperationCompleted(object sender, OperationCompletedEventArgs e)
        {
            try
            {
                // End the asynchronous join network session operation.
                NetworkSession networkSession = NetworkSession.EndJoin(e.AsyncResult);

                // Create a component that will manage the session we just joined.
                NetworkSessionComponent.Create(ScreenManager, networkSession);

                // Go to the lobby screen.
                ScreenManager.AddScreen(new LobbyScreen(networkSession));

                this.availableSessions.Dispose();
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

        /// <summary>
        /// Event handler for when the Back menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event arguments</param>
        private void BackMenuEntrySelected(object sender, EventArgs e)
        {
            this.availableSessions.Dispose();

            ExitScreen();
        }

        #endregion
    }
}
