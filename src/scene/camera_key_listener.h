#ifndef CAMERA_KEY_LISTENER_H
#define CAMERA_KEY_LISTENER_H

#include <functional>
#include <item2d/tg_item2d.h>

/*!
 * \brief CameraKeyListener
 * Listens key presses in handleEvent(). Key 0 selects the base camera (index 0)
 * and keys 1-6 select the camera objects (index 1-6 = camera object 0-5).
 * Key l toggles the direction line and key c toggles the camera object cones.
 * Key s saves the images of the camera objects.
 * Left and right arrow keys turn the camera while they are down
 * (direction -1 is left and 1 is right), up and down arrow keys turn
 * the camera up or down (direction -1 is down and 1 is up).
 * TgMainWindow sends the events only to its children, so this must be
 * a child of the main window.
 */
class CameraKeyListener : public TgItem2d
{
public:
    explicit CameraKeyListener(TgItem2d *parent);

    void connectOnCameraSelected(std::function<void(int index)> cameraSelected);
    void connectOnToggleDirectionLine(std::function<void()> toggleDirectionLine);
    void connectOnToggleCameraObjects(std::function<void()> toggleCameraObjects);
    void connectOnSaveImages(std::function<void()> saveImages);
    void connectOnTurnStart(std::function<void(int direction)> turnStart);
    void connectOnTurnStop(std::function<void(int direction)> turnStop);
    void connectOnPitchStart(std::function<void(int direction)> pitchStart);
    void connectOnPitchStop(std::function<void(int direction)> pitchStop);

protected:
    virtual TgEventResult handleEvent(TgEventData *eventData, const TgWindowInfo *windowInfo) override;

private:
    std::function<void(int index)> m_cameraSelected;
    std::function<void()> m_toggleDirectionLine;
    std::function<void()> m_toggleCameraObjects;
    std::function<void()> m_saveImages;
    std::function<void(int direction)> m_turnStart;
    std::function<void(int direction)> m_turnStop;
    std::function<void(int direction)> m_pitchStart;
    std::function<void(int direction)> m_pitchStop;
};

#endif // CAMERA_KEY_LISTENER_H
