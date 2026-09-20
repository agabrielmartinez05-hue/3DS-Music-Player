#include <3ds.h>
#include <citro2d.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_FILES 100
#define FILE_NAME_SIZE 256

// Scene Management (0 = FILE LIST, 1 = PLAYER VIEW)
int currentScene = 0;

// Playlist State Variables
char fileList[MAX_FILES][FILE_NAME_SIZE];
int totalFiles = 0;
int selectedIndex = 0;

// Playback Control Variables
Mix_Music* currentSong = NULL;
bool isLooping = true;
bool isPlaying = false;
char currentSongName[FILE_NAME_SIZE] = "No song selected";
int estimatedDurationSeconds = 180; // Default estimated duration (3 mins)

// Clock Variables for 3DS OS Timer
u64 startMs = 0;
u64 pausedMs = 0;
u64 pauseMomentMs = 0;

/**
 * Removes file extensions (.wav, .ogg, .mp3) for clean UI display.
 */
void stripExtension(char* dest, const char* src, size_t destSize) {
    snprintf(dest, destSize, "%s", src);
    char* dot = strrchr(dest, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
}

/**
 * Calculates current playback position in seconds based on system ticks.
 */
double getCurrentTime() {
    if (!isPlaying) return 0.0;
    
    if (Mix_PausedMusic()) {
        return (double)(pauseMomentMs - startMs - pausedMs) / 1000.0;
    }
    
    u64 currentMs = osGetTime();
    return (double)(currentMs - startMs - pausedMs) / 1000.0;
}

/**
 * Scans the SD card directory (/music) and loads compatible audio files (.ogg, .mp3, .wav).
 */
void loadMusicList() {
    DIR* dir = opendir("sdmc:/music");
    struct dirent* ent;
    totalFiles = 0;

    if (dir != NULL) {
        while ((ent = readdir(dir)) != NULL && totalFiles < MAX_FILES) {
            if (strstr(ent->d_name, ".ogg") || strstr(ent->d_name, ".mp3") || strstr(ent->d_name, ".wav")) {
                snprintf(fileList[totalFiles], FILE_NAME_SIZE, "%s", ent->d_name);
                totalFiles++;
            }
        }
        closedir(dir);
    } else {
        snprintf(fileList[0], FILE_NAME_SIZE, "Create /music folder on SD");
        totalFiles = 1;
    }
}

/**
 * Stops current audio track and loads the selected track from playlist.
 */
void playSelectedTrack() {
    if (totalFiles == 0) return;

    if (currentSong != NULL) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentSong);
        currentSong = NULL;
    }

    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "sdmc:/music/%s", fileList[selectedIndex]);

    currentSong = Mix_LoadMUS(fullPath);
    if (currentSong != NULL) {
        Mix_PlayMusic(currentSong, 1); // Play once; loop logic handled manually
        isPlaying = true;
        
        // Save cleaned name without extension for the UI
        stripExtension(currentSongName, fileList[selectedIndex], sizeof(currentSongName));
        
        // Reset playback timer
        startMs = osGetTime();
        pausedMs = 0;
    }
}

/**
 * Switches to next track in playlist.
 */
void nextTrack() {
    if (totalFiles <= 1) return;
    selectedIndex++;
    if (selectedIndex >= totalFiles) selectedIndex = 0;
    playSelectedTrack();
}

/**
 * Switches to previous track in playlist.
 */
void previousTrack() {
    if (totalFiles <= 1) return;
    selectedIndex--;
    if (selectedIndex < 0) selectedIndex = totalFiles - 1;
    playSelectedTrack();
}

int main(int argc, char** argv) {
    gfxInitDefault();
    
    // Prevent system from entering sleep mode when lid is closed
    aptSetSleepAllowed(false);

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Set render target exclusively to bottom screen
    C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    SDL_Init(SDL_INIT_AUDIO);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    C2D_TextBuf textBuf = C2D_TextBufNew(4096);

    loadMusicList();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;

        // Toggle scenes with SELECT button
        if (kDown & KEY_SELECT) {
            currentScene = (currentScene == 0) ? 1 : 0;
        }

        // Global Track Switch (L / R triggers)
        if (kDown & KEY_L) {
            previousTrack();
        }
        if (kDown & KEY_R) {
            nextTrack();
        }

        // Scene 0: Playlist Controls
        if (currentScene == 0) {
            if (kDown & KEY_DUP) {
                selectedIndex--;
                if (selectedIndex < 0) selectedIndex = totalFiles - 1;
            }
            if (kDown & KEY_DDOWN) {
                selectedIndex++;
                if (selectedIndex >= totalFiles) selectedIndex = 0;
            }
            if (kDown & KEY_A) {
                playSelectedTrack();
                currentScene = 1; // Switch to player UI upon track selection
            }
        }

        // Global Playback Controls
        if (kDown & KEY_Y) {
            if (isPlaying) {
                if (Mix_PausedMusic()) {
                    Mix_ResumeMusic();
                    pausedMs += (osGetTime() - pauseMomentMs);
                } else {
                    Mix_PauseMusic();
                    pauseMomentMs = osGetTime();
                }
            }
        }

        // Toggle Looping State (Button X)
        if (kDown & KEY_X) {
            isLooping = !isLooping;
        }

        // Automatic Track End & Looping Handler
        if (isPlaying && !Mix_PlayingMusic() && !Mix_PausedMusic()) {
            if (isLooping) {
                Mix_PlayMusic(currentSong, 1);
                startMs = osGetTime();
                pausedMs = 0;
            } else {
                nextTrack();
            }
        }

        // Bottom Screen Render Loop
        C2D_TextBufClear(textBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(bottomTarget, C2D_Color32(20, 20, 22, 255));
        C2D_SceneBegin(bottomTarget);

        // ==========================================
        // SCENE 0: FILE LIST VIEW
        // ==========================================
        if (currentScene == 0) {
            C2D_Text textHeader;
            C2D_TextParse(&textHeader, textBuf, "=== MUSIC PLAYLIST ===");
            C2D_TextOptimize(&textHeader);
            C2D_DrawText(&textHeader, C2D_WithColor, 10.0f, 10.0f, 0.5f, 0.55f, 0.55f, C2D_Color32(0, 255, 200, 255));

            for (int i = 0; i < totalFiles && i < 8; i++) {
                char cleanName[FILE_NAME_SIZE];
                stripExtension(cleanName, fileList[i], sizeof(cleanName));

                char menuItem[266];
                if (i == selectedIndex) {
                    snprintf(menuItem, sizeof(menuItem), "> %.35s", cleanName);
                } else {
                    snprintf(menuItem, sizeof(menuItem), "  %.35s", cleanName);
                }

                C2D_Text textItem;
                C2D_TextParse(&textItem, textBuf, menuItem);
                C2D_TextOptimize(&textItem);

                u32 textColor = (i == selectedIndex) ? C2D_Color32(255, 255, 0, 255) : C2D_Color32(200, 200, 200, 255);
                C2D_DrawText(&textItem, C2D_WithColor, 15.0f, 40.0f + (i * 20.0f), 0.5f, 0.5f, 0.5f, textColor);
            }

            C2D_Text textTip;
            C2D_TextParse(&textTip, textBuf, "[SELECT] Player UI | [A] Play | [L/R] Skip");
            C2D_TextOptimize(&textTip);
            C2D_DrawText(&textTip, C2D_WithColor, 10.0f, 215.0f, 0.5f, 0.45f, 0.45f, C2D_Color32(120, 120, 120, 255));
        }

        // ==========================================
        // SCENE 1: SPOTIFY-STYLE PLAYER VIEW
        // ==========================================
        else if (currentScene == 1) {
            // Title Header
            C2D_Text textLabel;
            C2D_TextParse(&textLabel, textBuf, "NOW PLAYING:");
            C2D_TextOptimize(&textLabel);
            C2D_DrawText(&textLabel, C2D_WithColor, 15.0f, 15.0f, 0.5f, 0.45f, 0.45f, C2D_Color32(30, 215, 96, 255));

            char croppedName[45];
            snprintf(croppedName, sizeof(croppedName), "%.35s", currentSongName);

            C2D_Text textTitle;
            C2D_TextParse(&textTitle, textBuf, croppedName);
            C2D_TextOptimize(&textTitle);
            C2D_DrawText(&textTitle, C2D_WithColor, 15.0f, 40.0f, 0.5f, 0.55f, 0.55f, C2D_Color32(255, 255, 255, 255));

            // Time calculation and clamping
            double currentTime = getCurrentTime();
            if (!isPlaying) {
                currentTime = 0.0;
            } else if (currentTime > estimatedDurationSeconds) {
                currentTime = (double)estimatedDurationSeconds;
            }

            // Progress percentage
            float percentage = (float)currentTime / (float)estimatedDurationSeconds;
            if (percentage > 1.0f) percentage = 1.0f;
            if (percentage < 0.0f) percentage = 0.0f;

            // Progress Bar Rendering
            float barX = 15.0f;
            float barY = 110.0f;
            float barWidth = 290.0f;
            float barHeight = 8.0f;

            C2D_DrawRectSolid(barX, barY, 0, barWidth, barHeight, C2D_Color32(60, 60, 60, 255));
            C2D_DrawRectSolid(barX, barY, 0, barWidth * percentage, barHeight, C2D_Color32(30, 215, 96, 255));

            // Formatted Time String
            char timeStr[32];
            int currentMin = (int)currentTime / 60;
            int currentSec = (int)currentTime % 60;
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d", currentMin, currentSec);

            C2D_Text textTime;
            C2D_TextParse(&textTime, textBuf, timeStr);
            C2D_TextOptimize(&textTime);
            C2D_DrawText(&textTime, C2D_WithColor, barX, barY + 15.0f, 0.5f, 0.45f, 0.45f, C2D_Color32(180, 180, 180, 255));

            // Status Indicator: PLAY / PAUSE
            const char* centerStatus = Mix_PausedMusic() ? "[ || PAUSE ]" : (isPlaying ? "[ > PLAYING ]" : "[ STOPPED ]");
            C2D_Text textStatus;
            C2D_TextParse(&textStatus, textBuf, centerStatus);
            C2D_TextOptimize(&textStatus);
            C2D_DrawText(&textStatus, C2D_WithColor, 115.0f, 160.0f, 0.5f, 0.55f, 0.55f, C2D_Color32(30, 215, 96, 255));

            // Status Indicator: REPEAT / LOOP
            const char* loopStatus = isLooping ? "LOOP: ON" : "LOOP: OFF";
            C2D_Text textLoop;
            C2D_TextParse(&textLoop, textBuf, loopStatus);
            C2D_TextOptimize(&textLoop);
            u32 loopColor = isLooping ? C2D_Color32(30, 215, 96, 255) : C2D_Color32(120, 120, 120, 255);
            C2D_DrawText(&textLoop, C2D_WithColor, 230.0f, 160.0f, 0.5f, 0.45f, 0.45f, loopColor);

            C2D_Text textTip;
            C2D_TextParse(&textTip, textBuf, "[SELECT] Playlist | [L/R] Skip | [Y] Pause | [X] Loop");
            C2D_TextOptimize(&textTip);
            C2D_DrawText(&textTip, C2D_WithColor, 10.0f, 215.0f, 0.5f, 0.45f, 0.45f, C2D_Color32(120, 120, 120, 255));
        }

        C3D_FrameEnd(0);
    }

    aptSetSleepAllowed(true);

    if (currentSong != NULL) {
        Mix_FreeMusic(currentSong);
    }
    C2D_TextBufDelete(textBuf);
    Mix_CloseAudio();
    SDL_Quit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}