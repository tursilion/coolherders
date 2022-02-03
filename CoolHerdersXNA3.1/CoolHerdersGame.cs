//-----------------------------------------------------------------------------
// <copyright file="CoolHerdersGame.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using CoolHerders.GameComponents;
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
    /// Holds the directions for looking up standard player motion
    /// </summary>
    public enum Direction
    {
        /// <summary>
        /// The player is facing or moving left
        /// </summary>
        DirectLeft = 0,

        /// <summary>
        /// The player is facing or moving down
        /// </summary>
        DirectDown,

        /// <summary>
        /// The player is facing or moving right
        /// </summary>
        DirectRight,

        /// <summary>
        /// The player is facing or moving left.
        /// </summary>
        DirectUp,
    }

    /// <summary>
    /// This is the main type for your game
    /// </summary>
    public class CoolHerdersGame : Microsoft.Xna.Framework.Game
    {
        /// <summary>
        /// This tracks the GraphicsDeviceManager for the game
        /// </summary>
        private GraphicsDeviceManager graphics;

        /// <summary>
        /// This tracks the ScreenManager for the game
        /// </summary>
        private ScreenManager screenManager;

        /// <summary>
        /// This provides a gobal Audio Engine for the game
        /// </summary>
        private AudioEngine gameAudioEngine;

        /// <summary>
        /// This provides a global background wave bank for all screens to share
        /// The warning is suppressed because we never use this, but MUST load it for audio to work
        /// </summary>
        [SuppressMessage("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields", Justification = "We must load this, but XACT never uses this reference")]
        private WaveBank backgroundWaveBank;

        /// <summary>
        /// A wave bank for our effects
        /// </summary>
        [SuppressMessage("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields", Justification = "We must load this, but XACT never uses this reference")]
        private WaveBank effectsWaveBank;

        /// <summary>
        /// This provides a global background sound bank for all screens to share
        /// </summary>
        private SoundBank backgroundSoundBank;

        /// <summary>
        /// The sound bank for the generic sound effects
        /// </summary>
        private SoundBank effectsSoundBank;

        /// <summary>
        /// This tracks the cue that is currently playing in the background
        /// </summary>
        private Cue backgroundCue;

        /// <summary>
        /// Tracks any gamer services that may be available
        /// </summary>
        private GamerServicesComponent gamerComponent;

        /// <summary>
        /// Initializes a new instance of the CoolHerdersGame class
        /// </summary>
        public CoolHerdersGame()
        {
            this.graphics = new GraphicsDeviceManager(this);
            Content.RootDirectory = "Content";

            this.graphics.PreferredBackBufferWidth = 1280;
            this.graphics.PreferredBackBufferHeight = 720;
#if !DEBUG
            this.graphics.IsFullScreen = true;
#endif

            this.IsFixedTimeStep = false;

            SafeAreasComponent safeAreas = new SafeAreasComponent(this);
            safeAreas.DrawOrder = 10;
            Components.Add(safeAreas);

            // Create the screen manager component.
            this.screenManager = new ScreenManager(this);
            this.screenManager.DrawOrder = 9;
            Components.Add(this.screenManager);

            Components.Add(new MessageDisplayComponent(this));
            try
            {
                this.gamerComponent = new GamerServicesComponent(this);
                Components.Add(this.gamerComponent);
            }
            catch (GamerServicesNotAvailableException)
            {
                GameSettings.Instance.GamerServicesActive = false;
            }

            this.gameAudioEngine = new AudioEngine("Content\\Audio\\CoolHerdersAudio.xgs");
            this.backgroundWaveBank = new WaveBank(this.gameAudioEngine, "Content\\Audio\\BackgroundMusic.xwb");
            this.backgroundSoundBank = new SoundBank(this.gameAudioEngine, "Content\\Audio\\BackgroundMusic.xsb");
            this.effectsWaveBank = new WaveBank(this.gameAudioEngine, "Content\\Audio\\SoundEffects.xwb");
            this.effectsSoundBank = new SoundBank(this.gameAudioEngine, "Content\\Audio\\SoundEffects.xsb");

            // Activate the first screens.
#if DEBUG
            this.screenManager.AddScreen(new TitleScreen());
#else
            this.screenManager.AddScreen(new HarmlessLogoScreen());
#endif
        }

        /// <summary>
        /// Gets the master audio engine for the entire game
        /// </summary>
        public AudioEngine GameAudioEngine
        {
            get
            {
                return this.gameAudioEngine;
            }
        }

        /// <summary>
        /// Allows a portion of the game to trigger a background audio file
        /// </summary>
        /// <param name="audioCueName">The name of the audio cue to play in the background</param>
        public void PlayBackgroundAudioCue(string audioCueName)
        {
            if (null != this.backgroundCue)
            {
                this.backgroundCue.Stop(AudioStopOptions.Immediate);
                this.backgroundCue = null;
            }

            this.backgroundCue = this.backgroundSoundBank.GetCue(audioCueName);
            this.backgroundCue.Play();
        }

        /// <summary>
        /// Plays a 'click' sound that's probably generally useful
        /// </summary>
        public void PlayClick()
        {
            this.effectsSoundBank.PlayCue("click");
        }

        /// <summary>
        /// Stops any currently playing background audio cue
        /// </summary>
        public void StopBackgroundAudioCue()
        {
            if (null != this.backgroundCue)
            {
                this.backgroundCue.Stop(AudioStopOptions.Immediate);
                this.backgroundCue = null;
            }
        }

        /// <summary>
        /// Checks to see if the background audio cue is playing
        /// </summary>
        /// <returns>Is the background audio cue playing</returns>
        public bool IsBackgroundAudioCuePlaying()
        {
            if (null != this.backgroundCue)
            {
                return this.backgroundCue.IsPlaying;
            }

            return false;
        }

        /// <summary>
        /// Initializes the game object, after all needed construction is done
        /// If GamerServices will not be available for this cycle, the component is removed
        /// </summary>
        protected override void Initialize()
        {
            try
            {
                base.Initialize();
            }
            catch (GamerServicesNotAvailableException)
            {
                GameSettings.Instance.GamerServicesActive = false;
                Components.Remove(this.gamerComponent);
            }
        }

        /// <summary>
        /// This is called when the game should draw itself.
        /// </summary>
        /// <param name="gameTime">Provides a snapshot of timing values.</param>
        protected override void Draw(GameTime gameTime)
        {
#if DEBUG
            this.graphics.GraphicsDevice.Clear(Color.CornflowerBlue);
            Debug.Assert(!gameTime.IsRunningSlowly, "The game started running slowly.");
#else
            this.graphics.GraphicsDevice.Clear(Color.Black);
#endif
            base.Draw(gameTime);
        }
    }
}
