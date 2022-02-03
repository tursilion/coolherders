//-----------------------------------------------------------------------------
// <copyright file="StoryModeSequencer.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.StoryMode
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text;
    using CoolHerders.Housekeeping;
    using CoolHerders.LevelDescriptions;
    using CoolHerders.Networking;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    using Microsoft.Xna.Framework.Storage;

    /// <summary>
    /// This is the main sequencer for the story mode.
    /// It is responsible for triggering the display of all story graphics, as well as configuring the levels, and monitoring wins/losses
    /// It will also handle the trial mode, simply by running a different (shortened) script
    /// </summary>
    internal class StoryModeSequencer
    {
        /// <summary>
        /// The instructions (for the machine) to perform
        /// </summary>
        private List<String> storyModeInstructions = new List<string>();

        /// <summary>
        /// The caption to show when performing that instruction (may be empty)
        /// </summary>
        private List<String> storyModeCaptions = new List<string>();

        /// <summary>
        /// The path in which story mode scripts can be found
        /// </summary>
        // XNA4.0 - TitleLocation no longer exists
        //private readonly string storyFileLocation = Path.Combine(StorageContainer.TitleLocation, "StoryMode");
        private readonly string storyFileLocation = Path.Combine("", "StoryMode");

        /// <summary>
        /// The location to the script that we have actually chosen to run
        /// </summary>
        private string storyModeFullLocation;

        /// <summary>
        /// A reference to the screen manager, since we will need to trigger the display of many screens
        /// </summary>
        private ScreenManager screenManager;

        private GameScreen storyModeSlate;

        /// <summary>
        /// Our 'program counter', showing which script instruction we are executing
        /// </summary>
        private int scriptIndex = 0;

        /// <summary>
        /// The level number on which we are playing
        /// </summary>
        private int levelNumber = 0;

        /// <summary>
        /// The number of rounds to play on this screen
        /// </summary>
        private int roundsToPlay = 0;

        /// <summary>
        /// The number of rounds that have been played
        /// </summary>
        private int roundsPlayed = 0;

        /// <summary>
        /// The number of rounds that have been won
        /// </summary>
        private int roundsWon = 0;

        private int startingLevel = 0;

        private int currentLevel = 0;

        private NetworkSession pseudoNetworkSession;

        /// <summary>
        /// Initializes an instance of the StoryModeSquencer class.  Also loads the story file in, parsing it as need be
        /// </summary>
        /// <param name="screenManager">The screen manager that handles flipping screens around</param>
        /// <param name="storyModeFile">Which story mode file to execute</param>
        public StoryModeSequencer(ScreenManager screenManager, NetworkSession networkSession, string storyModeFile, int startingLevel)
        {
            this.screenManager = screenManager;
            this.pseudoNetworkSession = networkSession;
            this.storyModeFullLocation = Path.Combine(this.storyFileLocation, storyModeFile);
            this.startingLevel = startingLevel;
            ParseStoryModeFile();
            if (this.startingLevel > 0)
            {
                RunNextSequence(true, true);
            }
        }

        /// <summary>
        /// A function that stops the story in progress
        /// </summary>
        public void StopStory()
        {
            SignedInGamer.SignedOut -= new EventHandler<SignedOutEventArgs>(SignedInGamer_SignedOut);
            if (GameInformation.Instance.EnemyPlayers != null)
            {
                GameInformation.Instance.EnemyPlayers.Clear();
                GameInformation.Instance.EnemyPlayers = null;
            }
        }

        /// <summary>
        /// A function to run the next sequence.  Should be called when the game is searching for something to do next.
        /// </summary>
        /// <param name="skipNonPlayableItems">Skips items which have no play value</param>
        public void RunNextSequence(bool skipNonPlayableItems, bool skipPlayableItems)
        {
            bool foundRecognizedInstruction = false;

            SignedInGamer.SignedOut -= new EventHandler<SignedOutEventArgs>(SignedInGamer_SignedOut);
            while (!foundRecognizedInstruction)
            {
                string nextInstruction = this.storyModeInstructions[this.scriptIndex];
                int pictureNumber;
                try
                {
                    pictureNumber = Int32.Parse(nextInstruction, System.Globalization.NumberStyles.Integer, System.Globalization.NumberFormatInfo.InvariantInfo);
                    if (!skipNonPlayableItems)
                    {
                        if (this.storyModeSlate == null)
                        {
                            foreach (GameScreen screen in screenManager.GetScreens())
                            {
                                screen.ExitScreen();
                            }
                            SignedInGamer.SignedOut += new EventHandler<SignedOutEventArgs>(SignedInGamer_SignedOut);
                            this.storyModeSlate = new StoryModeSlateScreen(pictureNumber, this.storyModeCaptions[this.scriptIndex]);
                            screenManager.AddScreen(this.storyModeSlate);
                            foundRecognizedInstruction = true;
                            scriptIndex++;
                            continue;
                        }
                        else
                        {
                            ((StoryModeSlateScreen)this.storyModeSlate).ShowNextPicture(pictureNumber, this.storyModeCaptions[this.scriptIndex]);
                            foundRecognizedInstruction = true;
                            scriptIndex++;
                            continue;
                        }
                    }
                    else
                    {
                        this.scriptIndex++;
                        continue;
                    }
                }
                catch (FormatException)
                {
                    this.storyModeSlate = null;
                }

                if ((nextInstruction.StartsWith("S", StringComparison.InvariantCulture)) || (nextInstruction.StartsWith("P", StringComparison.InvariantCulture)))
                {
                    if (!skipPlayableItems)
                    {
                        if (this.roundsToPlay > 0)
                        {
                            if (PlayNextRound(nextInstruction))
                            {
                                continue;
                            }
                        }
                        else
                        {
                            PlayRound(nextInstruction);
                        }
                        foundRecognizedInstruction = true;
                    }
                    else
                    {
                        this.scriptIndex++;
                        continue;
                    }
                }
                else if (nextInstruction.StartsWith("B"))
                {
                    if (!skipNonPlayableItems)
                    {
                        LoadingScreen.Load(screenManager, false, new BlankScreen());
                        foundRecognizedInstruction = true;
                        this.scriptIndex++;
                    }
                    else
                    {
                        this.scriptIndex++;
                        continue;
                    }
                }
                else if (nextInstruction.StartsWith("L"))
                {
                    if (this.startingLevel > 0)
                    {
                        int testLevel = (int)char.GetNumericValue(nextInstruction[1]);
                        if (testLevel > this.startingLevel)
                        {
                            this.startingLevel = 0;
                            foundRecognizedInstruction = true;
                            continue;
                        }
                        this.scriptIndex++;
                        continue;
                    }
                    else
                    {
                        this.currentLevel = (int)char.GetNumericValue(nextInstruction[1]);
                        this.scriptIndex++;
                        continue;
                    }
                }
                else if (nextInstruction.StartsWith("E"))
                {
                    if (!skipNonPlayableItems)
                    {
                        PlayerSavedInformation savedInfo = ((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).SavedInfo;
                        savedInfo.lastCompletedLevel = this.currentLevel;
                        if (this.currentLevel > savedInfo.highestCompletedLevel)
                        {
                            savedInfo.highestCompletedLevel = this.currentLevel;
                        }
                        ((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).SavedInfo = savedInfo;
                        ((PlayerSaveManager)SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex].Tag).SaveGame();
                        this.scriptIndex++;
                        continue;
                    }
                    else
                    {
                        this.scriptIndex++;
                        continue;
                    }
                }
                else if (this.storyModeInstructions[this.scriptIndex] == "``")
                {
                    // We have reached the end of the file, load the main menu
                    NetworkSessionComponent.LeaveSession(this.screenManager, true);
                    LoadingScreen.Load(screenManager, false, new BackgroundScreen(), new MainMenuScreen());
                    foundRecognizedInstruction = true;
                    this.scriptIndex++;
                }
                else
                {
                    // Unknown instruction encountered... skip it
                    this.scriptIndex++;
                }
            }
        }

        /// <summary>
        /// Sets up and triggers play of a single round of the game, leaving breadcrumbs for future rounds
        /// </summary>
        /// <param name="nextInstruction">The command code from the script file</param>
        private void PlayRound(string nextInstruction)
        {
            char[] levelCode = nextInstruction.Substring(1).ToCharArray();
            if (GameInformation.Instance.EnemyPlayers == null)
            {
                GameInformation.Instance.EnemyPlayers = new List<PlayerInformation>();
            }
            GameInformation.Instance.EnemyPlayers.Clear();

            PlayerInformation singlePlayer = new PlayerInformation();
            singlePlayer.CharacterClass = "Zeus";
            singlePlayer.PlayerActive = true;
            singlePlayer.SeatNumber = 0;
            singlePlayer.PlayerColorIndex = 0;
            singlePlayer.SheepCurrentRound = 0;
            singlePlayer.SheepTotal = 0;
            singlePlayer.SignedInGamer = SignedInGamer.SignedInGamers[GameInformation.Instance.MasterPlayerIndex];
            try
            {
                singlePlayer.GamerProfile = singlePlayer.SignedInGamer.GetProfile();
            }
            catch (NetworkException)
            {
            }

            this.pseudoNetworkSession.LocalGamers[0].Tag = singlePlayer;

            this.roundsPlayed = 0;
            this.roundsWon = 0;
            if (nextInstruction.Length == 4)
            {
                this.levelNumber = (int)Char.GetNumericValue(levelCode[0]) - 1;
                this.roundsToPlay = (int)Char.GetNumericValue(levelCode[1]);
                int patternNumber = (int)Char.GetNumericValue(levelCode[2]) - 1;
                GameInformation.Instance.WorldScreen = LevelDescription.LoadLevelFromCode((BuiltInLevel)this.levelNumber);
                GameInformation.Instance.CurrentStageNumber = patternNumber;
            }
            else
            {
                this.levelNumber = (int)Char.GetNumericValue(levelCode[0]) - 1;
                this.roundsToPlay = (int)Char.GetNumericValue(levelCode[1]);
                GameInformation.Instance.WorldScreen = LevelDescription.LoadLevelFromCode((BuiltInLevel)levelNumber);
                GameInformation.Instance.CurrentStageNumber = this.roundsPlayed;
            }

            if (GameInformation.Instance.WorldScreen.ZeusWearsAFro)
            {
                singlePlayer.CharacterClass = "ZeusAfro";
            }

            foreach (PlayerInformation enemyPlayerInfo in GameInformation.Instance.WorldScreen.SinglePlayerEnemies)
            {
                GameInformation.Instance.EnemyPlayers.Add(enemyPlayerInfo);
            }

            LoadingScreen.Load(
                screenManager,
                true,
                new MazeScreen(this.pseudoNetworkSession));
        }

        /// <summary>
        /// Plays the next round, or continues with the script.  Also handles win/loss cases
        /// </summary>
        /// <param name="nextInstruction">The command code from the game script</param>
        /// <returns>True if the script should continue, false if this function has control</returns>
        private bool PlayNextRound(string nextInstruction)
        {
            char[] levelCode = nextInstruction.Substring(1).ToCharArray();

            this.roundsPlayed++;

            bool playerHasWon = true;
            foreach (PlayerInformation enemyPlayer in GameInformation.Instance.EnemyPlayers)
            {
                if (enemyPlayer.SheepCurrentRound > ((PlayerInformation)this.pseudoNetworkSession.LocalGamers[0].Tag).SheepCurrentRound)
                {
                    playerHasWon = false;
                }
            }

            if (playerHasWon)
            {
                this.roundsWon++;
            }

            float roundWinRatio = (float)this.roundsWon / (float)this.roundsToPlay;

            if ((roundWinRatio > 0.5f) || (GameSettings.Instance.ForceSinglePlayerWin))
            {
                // We have a winner
                this.roundsToPlay = 0;
                this.scriptIndex++;
                return true;
            }

            float maxRoundWinRatio = ((float)this.roundsWon + (float)(this.roundsToPlay - this.roundsPlayed)) / (float)this.roundsToPlay;

            if ((maxRoundWinRatio < 0.5f) || (this.roundsPlayed == this.roundsToPlay))
            {
                // We played all the rounds without finding a winner! You LOSE! Good DAY, Sir!
                NetworkSessionComponent.LeaveSession(this.screenManager, true);
                LoadingScreen.Load(screenManager, false, new BackgroundScreen(), new MainMenuScreen());
                return false;
            }
            else
            {
                if (nextInstruction.Length == 4)
                {
                    this.levelNumber = (int)Char.GetNumericValue(levelCode[0]) - 1;
                    this.roundsToPlay = (int)Char.GetNumericValue(levelCode[1]);
                    int patternNumber = (int)Char.GetNumericValue(levelCode[2]) - 1;
                    GameInformation.Instance.WorldScreen = LevelDescription.LoadLevelFromCode((BuiltInLevel)this.levelNumber);
                    GameInformation.Instance.CurrentStageNumber = patternNumber;
                }
                else
                {
                    this.levelNumber = (int)Char.GetNumericValue(levelCode[0]) - 1;
                    this.roundsToPlay = (int)Char.GetNumericValue(levelCode[1]);
                    GameInformation.Instance.WorldScreen = LevelDescription.LoadLevelFromCode((BuiltInLevel)levelNumber);
                    GameInformation.Instance.CurrentStageNumber = this.roundsPlayed;
                }
                LoadingScreen.Load(
                    screenManager,
                    true,
                    new MazeScreen(this.pseudoNetworkSession));
                return false;
            }
        }

        /// <summary>
        /// A helper function that loads the commands in from the file
        /// </summary>
        private void ParseStoryModeFile() {
            // XNA4.0 - TitleLocation doesn't exist anymore, must use TitleContainer API
            System.IO.Stream storyStream = TitleContainer.OpenStream(this.storyModeFullLocation);
            StreamReader streamReader = new StreamReader(storyStream);
            string nextCommandLine = streamReader.ReadLine();
            while (nextCommandLine != null)
            {
                nextCommandLine = nextCommandLine.Trim();
                if (nextCommandLine != string.Empty)
                {
                    if (!nextCommandLine.StartsWith("//", StringComparison.InvariantCulture))
                    {
                        int commandLength = Math.Min(nextCommandLine.Length, 5);
                        string nextCommand = nextCommandLine.Substring(0, commandLength).Trim();
                        storyModeInstructions.Add(nextCommand);
                        if (nextCommandLine.Length > commandLength)
                        {
                            string nextCaption = nextCommandLine.Substring(commandLength).Trim();
                            storyModeCaptions.Add(nextCaption);
                        }
                        else
                        {
                            storyModeCaptions.Add(string.Empty);
                        }
                    }
                }
                nextCommandLine = streamReader.ReadLine();
            }
        }

        /// <summary>
        /// Called when a gamer has signed out during single player mode
        /// Checks if the single player has signed out, and then aborts the game if needed
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">The gamer who signed out</param>
        private void SignedInGamer_SignedOut(object sender, SignedOutEventArgs e)
        {
            if (e.Gamer.PlayerIndex == GameInformation.Instance.MasterPlayerIndex)
            {
                NetworkSessionComponent.LeaveSession(this.screenManager, true);
                LoadingScreen.Load(screenManager, false, new BackgroundScreen(), new MainMenuScreen());
            }
        }
    }
}
