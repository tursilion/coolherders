//-----------------------------------------------------------------------------
// <copyright file="PlayerInformation.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.Xna.Framework.GamerServices;

    /// <summary>
    /// Keeps track of all player information specific to this game
    /// Versus, of course, any Gamer information from Microsoft
    /// Or any moment to moment information used inside the maze game component
    /// </summary>
    public class PlayerInformation
    {
        /// <summary>
        /// Is this player active?
        /// </summary>
        private bool active;

        /// <summary>
        /// What is the class name of this player's chosen character
        /// </summary>
        private string characterClass;

        /// <summary>
        /// The profile for this gamer, set from their PlayerIndex when they sign in and out
        /// </summary>
        private GamerProfile profile;

        /// <summary>
        /// The signed in gamer, in case we have one, so that we can look backwards if we wish to
        /// </summary>
        private SignedInGamer signedInGamer;

        /// <summary>
        /// What seat number is this player in
        /// </summary>
        private byte playerSeatNumber;

        /// <summary>
        /// Which color table should we read for this player?
        /// </summary>
        private byte playerColorIndex;

        /// <summary>
        /// The number of sheep the player acquired in the current round
        /// </summary>
        private int sheepCurrentRound;

        /// <summary>
        /// The number of sheep the player has acquired so far
        /// </summary>
        private int sheepTotal;

        /// <summary>
        /// The skill level of the player (initially AI, but further along the players may store this)
        /// </summary>
        private int skillLevel;

        /// <summary>
        /// Gets or sets a value indicating whether this player is active
        /// </summary>
        public bool PlayerActive
        {
            get
            {
                return this.active;
            }

            set
            {
                this.active = value;
            }
        }

        /// <summary>
        /// Gets or sets the number of sheep the player got this round
        /// </summary>
        public int SheepCurrentRound
        {
            get { return this.sheepCurrentRound; }
            set { this.sheepCurrentRound = value; }
        }

        /// <summary>
        /// Gets or sets the total number of sheep the player has acquired
        /// </summary>
        public int SheepTotal
        {
            get { return this.sheepTotal; }
            set { this.sheepTotal = value; }
        }

        /// <summary>
        /// Gets or sets the character class name of this player
        /// </summary>
        public string CharacterClass
        {
            get
            {
                return this.characterClass;
            }

            set
            {
                this.characterClass = value;
            }
        }

        /// <summary>
        /// Gets or sets the gamer profile for this player
        /// </summary>
        public GamerProfile GamerProfile
        {
            get
            {
                return this.profile;
            }

            set
            {
                this.profile = value;
            }
        }

        /// <summary>
        /// Gets or sets the seat number for this player
        /// </summary>
        public byte SeatNumber
        {
            get
            {
                return this.playerSeatNumber;
            }

            set
            {
                this.playerSeatNumber = value;
            }
        }

        /// <summary>
        /// Gets or sets which signed in gamer is referred to by this player info
        /// </summary>
        public SignedInGamer SignedInGamer
        {
            get
            {
                return this.signedInGamer;
            }

            set
            {
                this.signedInGamer = value;
            }
        }

        /// <summary>
        /// Gets or sets the player color index for loading of tile sheets
        /// </summary>
        public byte PlayerColorIndex
        {
            get
            {
                return this.playerColorIndex;
            }

            set
            {
                this.playerColorIndex = value;
            }
        }

        public int SkillLevel
        {
            get { return skillLevel; }
            set { skillLevel = value; }
        }
    }
}
