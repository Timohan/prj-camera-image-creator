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

protected:
    virtual TgEventResult handleEvent(TgEventData *eventData, const TgWindowInfo *windowInfo) override;

private:
    std::function<void(int index)> m_cameraSelected;
    std::function<void()> m_toggleDirectionLine;
    std::function<void()> m_toggleCameraObjects;
    std::function<void()> m_saveImages;
};

#endif // CAMERA_KEY_LISTENER_H
