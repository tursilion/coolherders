//-----------------------------------------------------------------------------
// <copyright file="NewZealandLevel.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.LevelDescriptions
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// Compiled in level for the New Zealand level
    /// </summary>
    public class NewZealandLevel : LevelDescription
    {
        /// <summary>
        /// Initializes a new instance of the NewZealandLevel class
        /// </summary>
        /// <param name="mapNumber">The map number from this level we are working with</param>
        public NewZealandLevel()
        {
            this.theLevelName = "nz";
            this.thePrintableName = "New Zealand";
            this.theLevelNumber = 0;
            this.theBackgroundSongName = "NZ2Background";
            this.singlePlayerEnemies = new List<PlayerInformation>();
            this.sheepAreGhosts = false;
            this.zeusWearsAFro = false;

            PlayerInformation enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Classic";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 1;
            enemyPlayer.PlayerColorIndex = 0;
            this.SinglePlayerEnemies.Add(enemyPlayer);
        }

        public override bool IsLevelUnlocked
        {
            get
            {
                return true;
            }
        }
    }
}
