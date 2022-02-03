//-----------------------------------------------------------------------------
// <copyright file="CandyLevel.cs" company="HarmlessLion">
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
    /// This is a compiled in level for CandyLand
    /// </summary>
    public class CandyLevel : LevelDescription
    {
        /// <summary>
        /// Initializes a new instance of the CandyLevel class
        /// </summary>
        /// <param name="mapNumber">The map number from this level we are working with</param>
        public CandyLevel()
        {
            this.theLevelName = "candy";
            this.thePrintableName = "Candy Shoppe";
            this.theLevelNumber = 1;
            this.theBackgroundSongName = "CandylandBackground";
            this.sheepAreGhosts = false;
            this.zeusWearsAFro = false;

            this.singlePlayerEnemies = new List<PlayerInformation>();

            PlayerInformation enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Candy";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 1;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 1;
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
