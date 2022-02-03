//-----------------------------------------------------------------------------
// <copyright file="GameSettings.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    /// Holds all the current settings for our game
    /// </summary>
    internal sealed class GameSettings
    {
        /// <summary>
        /// Initializes the singleton instance when needed
        /// </summary>
        private static readonly GameSettings instance = new GameSettings();

        /// <summary>
        /// Set to true if we have Gamer Services available (PC demos do not)
        /// </summary>
        private bool canHasGamerServices;

        /// <summary>
        /// The minimum number of herders to have in the game
        /// </summary>
        private int minimumNumberOfHerders;

        /// <summary>
        /// Force a win in single player mode
        /// </summary>
        private bool forceSinglePlayerWin;

        /// <summary>
        /// Prevents a default instance of the GameSettings class from being created.
        /// </summary>
        private GameSettings()
        {
            this.canHasGamerServices = true;
            this.minimumNumberOfHerders = 1;
            this.forceSinglePlayerWin = false;
        }

        /// <summary>
        /// Gets the singleton instance
        /// </summary>
        public static GameSettings Instance
        {
            get
            {
                return instance;
            }
        }

        /// <summary>
        /// Gets or sets the minimum number of herders to put in a game
        /// </summary>
        public int MinimumNumberOfHerders
        {
            get
            {
                return this.minimumNumberOfHerders;
            }

            set
            {
                this.minimumNumberOfHerders = value;
            }
        }

        public bool ForceSinglePlayerWin
        {
            get
            {
                return this.forceSinglePlayerWin;
            }

            set
            {
                this.forceSinglePlayerWin = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether gamer services are active
        /// </summary>
        public bool GamerServicesActive
        {
            get
            {
                return this.canHasGamerServices;
            }

            set
            {
                this.canHasGamerServices = value;
            }
        }
    }
}
