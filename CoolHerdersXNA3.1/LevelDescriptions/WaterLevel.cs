//-----------------------------------------------------------------------------
// <copyright file="WaterLevel.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.LevelDescriptions
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;
    using Microsoft.Xna.Framework.GamerServices;

    /// <summary>
    /// This is a compiled in level for the Water Works
    /// </summary>
    public class WaterLevel : LevelDescription
    {
        /// <summary>
        /// Initializes a new instance of the WaterLevel class
        /// </summary>
        /// <param name="mapNumber">The map number from this level we are working with</param>
        public WaterLevel()
        {
            this.theLevelName = "water";
            this.thePrintableName = "Water Works";
            this.theLevelNumber = 5;
            this.theBackgroundSongName = "WaterworksBackground";
            this.sheepAreGhosts = false;
            this.zeusWearsAFro = false;

            this.singlePlayerEnemies = new List<PlayerInformation>();

            PlayerInformation enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Iskur";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 1;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 7;
            this.SinglePlayerEnemies.Add(enemyPlayer);
        }

        public override bool IsLevelUnlocked
        {
            get
            {
                return LockByHighestLevel();
            }
        }
    }
}
