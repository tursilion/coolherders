//-----------------------------------------------------------------------------
// <copyright file="HellLevel.cs" company="HarmlessLion">
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
    /// This is a compiled in level for Hell
    /// </summary>
    public class HellLevel : LevelDescription
    {
        /// <summary>
        /// Initializes a new instance of the HellLevel class
        /// </summary>
        /// <param name="mapNumber">The map number from this level we are working with</param>
        public HellLevel()
        {
            this.theLevelName = "hell";
            this.thePrintableName = "Hell";
            this.theLevelNumber = 7;
            this.theBackgroundSongName = "HellBackground";
            this.sheepAreGhosts = false;
            this.zeusWearsAFro = false;

            this.singlePlayerEnemies = new List<PlayerInformation>();

            PlayerInformation enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Demon";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 1;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 7;
            this.SinglePlayerEnemies.Add(enemyPlayer);
            enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Demon";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 2;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 8;
            this.SinglePlayerEnemies.Add(enemyPlayer);
            enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "Demon";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 3;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 8;
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
