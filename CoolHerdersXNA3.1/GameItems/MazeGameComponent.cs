//-----------------------------------------------------------------------------
// <copyright file="MazeGameComponent.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------
namespace CoolHerders
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using CoolHerders.Housekeeping;
    using CoolHerders.GameItems;
    using CoolHerders.LevelDescriptions;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Audio;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;
    using Microsoft.Xna.Framework.Net;
    using Microsoft.Xna.Framework.Storage;

    /// <summary>
    /// This is a game component that implements IUpdateable.
    /// </summary>
    internal class MazeGameComponent : Microsoft.Xna.Framework.DrawableGameComponent
    {
        /// <summary>
        /// The list of pickup items that are currently available
        /// </summary>
        public List<PickupItem> PickupItems;

        /// <summary>
        /// The player characters that will be running around our fine maze
        /// </summary>
        public PlayerCharacter[] PlayerCharacters;

        /// <summary>
        /// A source of random numbers
        /// </summary>
        public Random RandomNumber = new Random();

        /// <summary>
        /// Maze Items to which a player can walk, thus a cheat for the AI
        /// </summary>
        public List<MazeItem> PassableMazeItems;

        /// <summary>
        /// A list of maze items which can be destroyed
        /// </summary>
        public List<DestructableMazeItem> DestructableMazeItems;

        /// <summary>
        /// The game screen we are running on
        /// </summary>
        public MazeScreen GameScreen;

        /// <summary>
        /// The number of sheep to put into the level
        /// </summary>
        private const int NumberOfSheepies = 29;

        /// <summary>
        /// The tile positions at which the four players would start
        /// </summary>
        private static readonly Vector2[] playerStarts = new Vector2[4]
        {
            new Vector2(2.0f, 2.0f),
            new Vector2(18.0f, 2.0f),
            new Vector2(2.0f, 12.0f),
            new Vector2(18.0f, 12.0f),
        };

        /// <summary>
        /// A generic list holding all of the maze items that need drawing
        /// </summary>
        private List<MazeItem> mazeItemArray;

        /// <summary>
        /// The maze items where a sheep can start out
        /// </summary>
        private List<MazeItem> sheepOkMazeItems;

        /// <summary>
        /// The sprite batch into which all of the maze drawing is done
        /// </summary>
        private SpriteBatch mazeSpriteBatch;

        /// <summary>
        /// A private content manager that holds all content we may load at this point
        /// </summary>
        private ContentManager playGameContent;

        /// <summary>
        /// Tracks any and all sheep in existence
        /// </summary>
        private SheepCharacter[] sheepCharacter;

        /// <summary>
        /// The input state for the maze as a whole
        /// Can each player someday gather these if they want?
        /// </summary>
        private InputState mazeInputState;

        /// <summary>
        /// The screen manager that is responsible for drawing us
        /// </summary>
        private ScreenManager screenManager;

        /// <summary>
        /// The definition of the actual maze that we are going to draw
        /// </summary>
        private MazeMap mazeMap;

        /// <summary>
        /// A render target which will hold the content of the old screen
        /// </summary>
        private RenderTarget2D oldScreen;

        /// <summary>
        /// The description object for the current level
        /// </summary>
        private LevelDescription currentLevel;

        /// <summary>
        /// The number of free sheep in the level
        /// </summary>
        private int freeSheep = NumberOfSheepies;

        /// <summary>
        /// The packet reader for the game
        /// </summary>
        private PacketReader gamePacketReader = new PacketReader();

        /// <summary>
        /// The packet writer for the game
        /// </summary>
        private PacketWriter gamePacketWriter = new PacketWriter();

        /// <summary>
        /// The amount of time since a network update occurred
        /// </summary>
        private double timeSinceNetworkUpdate = 0.0f;

        /// <summary>
        /// Initializes a new instance of the MazeGameComponent class
        /// </summary>
        /// <param name="game">The game object we are running under</param>
        /// <param name="screen">The screen we are being drawn as part of</param>
        internal MazeGameComponent(Game game, MazeScreen screen)
            : base(game)
        {
            this.playGameContent = screen.Content;
            this.currentLevel = screen.CurrentLevel;
            this.screenManager = screen.ScreenManager;
            this.GameScreen = screen;
            // XNA4.0 - changes constructor
            //this.oldScreen = new RenderTarget2D(this.screenManager.GraphicsDevice, 672, 480, 0, this.screenManager.GraphicsDevice.PresentationParameters.BackBufferFormat);
            this.oldScreen = new RenderTarget2D(this.screenManager.GraphicsDevice, 672, 480);
            this.PickupItems = new List<PickupItem>();
            this.DestructableMazeItems = new List<DestructableMazeItem>();
            GameInformation.Instance.GamePacketWriter = this.gamePacketWriter;
            GameInformation.Instance.GamePacketReader = this.gamePacketReader;
        }

        /// <summary>
        /// Gets a value indicating the number of sheep freely roaming around
        /// </summary>
        public int FreeSheep
        {
            get
            {
                return this.freeSheep;
            }
        }

        /// <summary>
        /// Gets the screen manager responsible for drawing the maze
        /// </summary>
        public ScreenManager ScreenManager
        {
            get
            {
                return this.screenManager;
            }
        }

        /// <summary>
        /// Gets the maze input state
        /// </summary>
        public InputState MazeInputState
        {
            get
            {
                return this.mazeInputState;
            }
        }

        /// <summary>
        /// Gets the content manager for this component
        /// </summary>
        public ContentManager PlayGameContent
        {
            get
            {
                return this.playGameContent;
            }
        }

        /// <summary>
        /// Allows the game component to perform any initialization it needs to before starting
        /// to run.  This is where it can query for any required services and load content.
        /// </summary>
        public override void Initialize()
        {
            this.mazeInputState = (InputState)Game.Services.GetService(typeof(IInputState));

            base.Initialize();
        }

        /// <summary>
        /// Allows the game component to update itself.
        /// </summary>
        /// <param name="gameTime">Provides a snapshot of timing values.</param>
        public override void Update(GameTime gameTime)
        {
            if (this.GameScreen.IsActive && (this.MazeInputState.CurrentGamePadStates[0].Buttons.RightShoulder == ButtonState.Released))
            {
                bool needNetworkUpdate = false;
                timeSinceNetworkUpdate += gameTime.ElapsedGameTime.Milliseconds;
                if (timeSinceNetworkUpdate > 100.0f)
                {
                    timeSinceNetworkUpdate -= 100.0f;
                    needNetworkUpdate = true;
                }

                foreach (LocalNetworkGamer gamer in this.GameScreen.NetSession.LocalGamers)
                {
                    int playerSeat = ((PlayerInformation)gamer.Tag).SeatNumber;
                    this.PlayerCharacters[playerSeat].Update(gameTime, this.PlayerCharacters, this.sheepCharacter);
                    if (needNetworkUpdate)
                    {
                        this.PlayerCharacters[playerSeat].UpdateNetworkStatus();
                        gamer.SendData(this.gamePacketWriter, SendDataOptions.InOrder);
                    }
                    if (gamer.IsDataAvailable)
                    {
                        NetworkGamer sendingPlayer;
                        gamer.ReceiveData(gamePacketReader, out sendingPlayer);
                        if (sendingPlayer.IsLocal)
                        {
                            continue;
                        }

                        int remotePlayerSeat = ((PlayerInformation)sendingPlayer.Tag).SeatNumber;
                        ((NetworkPlayerCharacter)this.PlayerCharacters[remotePlayerSeat]).DecodeNetworkPacket();
                    }
                }

                foreach (NetworkGamer gamer in this.GameScreen.NetSession.RemoteGamers)
                {
                    int remotePlayerSeat = ((PlayerInformation)gamer.Tag).SeatNumber;
                    this.PlayerCharacters[remotePlayerSeat].Update(gameTime, this.PlayerCharacters, this.sheepCharacter);
                }

                foreach (PlayerInformation playerInfo in this.GameScreen.AdditionalPlayers)
                {
                    int playerSeat = playerInfo.SeatNumber;
                    this.PlayerCharacters[playerSeat].Update(gameTime, this.PlayerCharacters, this.sheepCharacter);
                }

                this.freeSheep = 0;

                foreach (SheepCharacter sheepie in this.sheepCharacter)
                {
                    sheepie.Update(gameTime, this.PlayerCharacters, null);
                    if (sheepie.CurrentState != SheepCharacter.SheepState.sheepFollowing)
                    {
                        this.freeSheep++;
                    }
                }

                foreach (MazeItem mazeItem in this.mazeItemArray)
                {
                    if (mazeItem != null)
                    {
                        mazeItem.Update(gameTime);
                    }
                }

                foreach (PickupItem pickupItem in this.PickupItems)
                {
                    pickupItem.Update(gameTime);
                }
            }

            base.Update(gameTime);
        }

        /// <summary>
        /// Draws the maze screen and everything in the maze.
        /// </summary>
        /// <param name="gameTime">A GameTime class indicating the progression of time in the game</param>
        public override void Draw(GameTime gameTime)
        {
            // XNA4.0 changed
            //this.screenManager.GraphicsDevice.SetRenderTarget(0, this.oldScreen);
            this.screenManager.GraphicsDevice.SetRenderTarget(this.oldScreen);

            // XNA4.0 changed
            //this.mazeSpriteBatch.Begin(SpriteBlendMode.AlphaBlend, SpriteSortMode.FrontToBack, SaveStateMode.None);
            this.mazeSpriteBatch.Begin(SpriteSortMode.FrontToBack, BlendState.AlphaBlend);

            foreach (MazeItem mazeItem in this.mazeItemArray)
            {
                if (null != mazeItem)
                {
                    mazeItem.Draw(gameTime, this.mazeSpriteBatch, this.GameScreen.TransitionAlpha);
                }
            }

            foreach (PlayerCharacter player in this.PlayerCharacters)
            {
                if (player != null)
                {
                    player.Draw(this.mazeSpriteBatch, this.GameScreen.TransitionAlpha, 255);
                }
            }

            foreach (SheepCharacter sheepie in this.sheepCharacter)
            {
                sheepie.Draw(this.mazeSpriteBatch, this.GameScreen.TransitionAlpha, 255);
            }

            foreach (PickupItem pickupItem in this.PickupItems)
            {
                pickupItem.Draw(this.mazeSpriteBatch, this.GameScreen.TransitionAlpha, 255);
            }

            this.mazeSpriteBatch.End();

            // XNA 4.0
            //this.screenManager.GraphicsDevice.SetRenderTarget(0, null);
            this.screenManager.GraphicsDevice.SetRenderTarget(null);

            this.screenManager.GraphicsDevice.Clear(Color.Black);

            // XNA4.0 inherits this now
            //Texture2D oldScreenTexture = this.oldScreen.GetTexture();

            // XNA 4.0
            //this.mazeSpriteBatch.Begin(SpriteBlendMode.AlphaBlend, SpriteSortMode.Immediate, SaveStateMode.None, Matrix.CreateScale(1.5f, 1.5f, 1.0f));
            this.mazeSpriteBatch.Begin(SpriteSortMode.Immediate, null, null, null, null, null, Matrix.CreateScale(1.5f, 1.5f, 1.0f));

            this.mazeSpriteBatch.Draw(this.oldScreen, new Rectangle(0, 0, 672, 480), Color.White);

            this.mazeSpriteBatch.End();
        }

        /// <summary>
        /// Gets the number of sheep that are following a given player
        /// </summary>
        /// <param name="playerSeatNumber">The index of the player to query</param>
        /// <returns>The number of sheep that are following that player</returns>
        public int GetNumberOfSheep(int playerSeatNumber)
        {
            return this.PlayerCharacters[playerSeatNumber].NumberOfSheep;
        }

        /// <summary>
        /// Gets a reference to the maze item at the given row and column.
        /// </summary>
        /// <param name="row">The zero based row to retreive</param>
        /// <param name="column">The zero based column to retreive</param>
        /// <returns>A MazeItem</returns>
        internal MazeItem GetMazeItem(int row, int column)
        {
            return this.mazeItemArray[(row * 21) + column];
        }

        /// <summary>
        /// Load your graphics content.
        /// </summary>
        protected override void LoadContent()
        {
            this.mazeSpriteBatch = new SpriteBatch(GraphicsDevice);

            this.mazeMap = this.playGameContent.Load<MazeMap>(this.currentLevel.GetLevelMapName(GameInformation.Instance.CurrentStageNumber));

            this.PlayerCharacters = new PlayerCharacter[4];

            foreach (NetworkGamer gamer in this.GameScreen.NetSession.LocalGamers)
            {
                int seatNumber = ((PlayerInformation)gamer.Tag).SeatNumber;
                this.PlayerCharacters[seatNumber] = new PlayerCharacter(this, gamer, ((PlayerInformation)gamer.Tag), (int)playerStarts[seatNumber].Y, (int)playerStarts[seatNumber].X);
            }

            foreach (NetworkGamer gamer in this.GameScreen.NetSession.RemoteGamers)
            {
                int seatNumber = ((PlayerInformation)gamer.Tag).SeatNumber;
                this.PlayerCharacters[seatNumber] = new NetworkPlayerCharacter(this, gamer, ((PlayerInformation)gamer.Tag), (int)playerStarts[seatNumber].Y, (int)playerStarts[seatNumber].X);
            }

            foreach (PlayerInformation playerInfo in this.GameScreen.AdditionalPlayers)
            {
                int seatNumber = playerInfo.SeatNumber;

                if (playerInfo.SignedInGamer == null)
                {
                    this.PlayerCharacters[seatNumber] = new EnemyHerderCharacter(this, this.GameScreen.NetSession.LocalGamers[0], playerInfo, (int)playerStarts[seatNumber].Y, (int)playerStarts[seatNumber].X);
                }
                else
                {
                    this.PlayerCharacters[seatNumber] = new PlayerCharacter(this, this.GameScreen.NetSession.LocalGamers[0], playerInfo, (int)playerStarts[seatNumber].Y, (int)playerStarts[seatNumber].X);
                }
            }

            this.sheepCharacter = new SheepCharacter[NumberOfSheepies];

            this.mazeItemArray = new List<MazeItem>(15 * 21);
            this.PassableMazeItems = new List<MazeItem>(15 * 21);
            this.sheepOkMazeItems = new List<MazeItem>(15 * 21);

            for (int rowCounter = 0; rowCounter < 15; rowCounter++)
            {
                for (int columnCounter = 0; columnCounter < 21; columnCounter++)
                {
                    MazeItem item = MazeItem.MakeMazeItem(this, this.currentLevel.LevelName, this.currentLevel.LevelNumber, this.mazeMap.MazeTiles[(rowCounter * 21) + columnCounter], rowCounter, columnCounter);
                    this.mazeItemArray.Add(item);
                    if (item.IsPassable)
                    {
                        this.PassableMazeItems.Add(item);
                    }

                    if (item.Destructible)
                    {
                        this.DestructableMazeItems.Add((DestructableMazeItem)item);
                    }
                }
            }

            this.FindSheepStartingPositions();

            this.sheepCharacter = new SheepCharacter[NumberOfSheepies];

            for (int counter = 0; counter < NumberOfSheepies; counter++)
            {
                MazeItem item = this.sheepOkMazeItems[this.RandomNumber.Next(0, this.sheepOkMazeItems.Count - 1)];
                PlayerInformation playerInfo = new PlayerInformation();
                playerInfo.CharacterClass = "Sheep";
                playerInfo.PlayerColorIndex = 255;
                this.sheepCharacter[counter] = new SheepCharacter(this, this.GameScreen.NetSession.LocalGamers[0], playerInfo, item.RowPosition, item.ColumnPosition);
            }

            for (int counter = 0; counter < 4; counter++)
            {
                int crateNumber;
                do 
                {
                    crateNumber = this.RandomNumber.Next(this.DestructableMazeItems.Count);
                }
                while (this.DestructableMazeItems[crateNumber].HiddenItem != null);
                DestructableMazeItem selectedCrate = this.DestructableMazeItems[crateNumber];
                selectedCrate.HiddenItem = new ZapperPickup(this, this.GameScreen.NetSession.LocalGamers[0], selectedCrate.RowPosition, selectedCrate.ColumnPosition);
            }

            for (int counter = 0; counter < 8; counter++)
            {
                int crateNumber;
                do
                {
                    crateNumber = this.RandomNumber.Next(this.DestructableMazeItems.Count);
                }
                while (this.DestructableMazeItems[crateNumber].HiddenItem != null);
                DestructableMazeItem selectedCrate = this.DestructableMazeItems[crateNumber];
                selectedCrate.HiddenItem = new SpeedPickup(this, this.GameScreen.NetSession.LocalGamers[0], selectedCrate.RowPosition, selectedCrate.ColumnPosition);
            }
        }

        /// <summary>
        /// Unloads all content, and undoes anything else done at load time
        /// </summary>
        protected override void UnloadContent()
        {
            base.UnloadContent();
        }

        /// <summary>
        /// Using a flood-fill algorithm, find a list of valid starting positions for the sheep
        /// </summary>
        private void FindSheepStartingPositions()
        {
            MazeItem startingMazeItem = this.PassableMazeItems[this.PassableMazeItems.Count / 2];
            int rowCheck = startingMazeItem.RowPosition;
            int columnCheck = startingMazeItem.ColumnPosition;

            Queue<MazeItem> mazeQueue = new Queue<MazeItem>();
            mazeQueue.Enqueue(startingMazeItem);

            while (mazeQueue.Count > 0)
            {
                MazeItem item = mazeQueue.Dequeue();
                rowCheck = item.RowPosition;
                columnCheck = item.ColumnPosition;

                int westColumn = columnCheck;
                int eastColumn = columnCheck;

                this.sheepOkMazeItems.Add(item);
                item.SheepCanWalk = true;

                while (this.mazeItemArray[(rowCheck * 21) + westColumn - 1].IsPassable)
                {
                    westColumn--;
                    this.sheepOkMazeItems.Add(this.mazeItemArray[(rowCheck * 21) + westColumn]);
                }

                while (this.mazeItemArray[(rowCheck * 21) + eastColumn + 1].IsPassable)
                {
                    eastColumn++;
                    this.sheepOkMazeItems.Add(this.mazeItemArray[(rowCheck * 21) + eastColumn]);
                }

                for (int check = westColumn; check <= eastColumn; check++)
                {
                    MazeItem checkNorth = this.mazeItemArray[((rowCheck - 1) * 21) + check];
                    if (checkNorth.IsPassable && !checkNorth.SheepCanWalk)
                    {
                        mazeQueue.Enqueue(this.mazeItemArray[((rowCheck - 1) * 21) + check]);
                    }

                    MazeItem checkSouth = this.mazeItemArray[((rowCheck + 1) * 21) + check];
                    if (checkSouth.IsPassable && !checkSouth.SheepCanWalk)
                    {
                        mazeQueue.Enqueue(this.mazeItemArray[((rowCheck + 1) * 21) + check]);
                    }
                }
            }
        }
    }
}