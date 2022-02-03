//-----------------------------------------------------------------------------
// <copyright file="AvailableSessionMenuEntry.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using System.Globalization;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// Helper class customizes the standard MenuEntry class
    /// for displaying AvailableNetworkSession objects.
    /// </summary>
    internal class AvailableSessionMenuEntry : MenuEntry
    {
        #region Fields

        /// <summary>
        /// Holds the network session object for this available session
        /// </summary>
        private AvailableNetworkSession availableSession;

        /// <summary>
        /// Set to true if we know the quality of service information
        /// </summary>
        private bool gotQualityOfService;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the AvailableSessionMenuEntry class.
        /// </summary>
        /// <param name="availableSession">The available network session to generate an entry for</param>
        public AvailableSessionMenuEntry(AvailableNetworkSession availableSession)
            : base(GetMenuItemText(availableSession))
        {
            this.availableSession = availableSession;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets the available network session corresponding to this menu entry.
        /// </summary>
        public AvailableNetworkSession AvailableSession
        {
            get { return this.availableSession; }
        }

        #endregion

        #region Update

        /// <summary>
        /// Updates the menu item text, adding information about the network
        /// quality of service as soon as that becomes available.
        /// </summary>
        /// <param name="screen">The screen that we are being drawn as part of</param>
        /// <param name="isSelected">Is this menu option selected</param>
        /// <param name="gameTime">The current GameTime of this game</param>
        public override void Update(
            MenuScreen screen,
            bool isSelected,
            GameTime gameTime)
        {
            base.Update(screen, isSelected, gameTime);

            // Quality of service data can take some time to query, so it will not
            // be filled in straight away when NetworkSession.Find returns. We want
            // to display the list of available sessions straight away, and then
            // fill in the quality of service data whenever that becomes available,
            // so we keep checking until this data shows up.
            if (screen.IsActive && !this.gotQualityOfService)
            {
                QualityOfService qualityOfService = this.availableSession.QualityOfService;

                if (qualityOfService.IsAvailable)
                {
                    TimeSpan pingTime = qualityOfService.AverageRoundtripTime;

                    Text += string.Format(CultureInfo.CurrentCulture, " - {0:0} ms", pingTime.TotalMilliseconds);

                    this.gotQualityOfService = true;
                }
            }
        }

        #endregion

        /// <summary>
        /// Formats session information to create the menu text string.
        /// </summary>
        /// <param name="session">The network session to get the menu item text for</param>
        /// <returns>The text of the menu item for this network session</returns>
        private static string GetMenuItemText(AvailableNetworkSession session)
        {
            int totalSlots = session.CurrentGamerCount +
                             session.OpenPublicGamerSlots;

            return string.Format(
                CultureInfo.CurrentCulture,
                "{0} ({1}/{2})",
                session.HostGamertag,
                session.CurrentGamerCount,
                totalSlots);
        }
    }
}
