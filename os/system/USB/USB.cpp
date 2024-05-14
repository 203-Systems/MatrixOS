#include "USB.h"
#include "MatrixOS.h"


// Located in usb_config.h, extern here to make them dynamic
uint8_t fifo_size[CONFIG_USBDEV_EP_NUM - 1];

namespace MatrixOS::USB
{
    vector<uint8_t> usb_descriptor;

    void Init()
    {

    }

    bool Inited() {
      return false; // TODO
    }

    bool Connected() {
        return connected;
    }

    void Begin(void) {
        
        // Generate USB descriptor
        uint8_t* desc = CompileDescriptor();
        // Initialize USB

        // Set FIFO size， change unused to 0. Maximum RX fifo 
        // This is used to pervent re initialization of the USB controller
        // when the endpoint is changed.
        // Drop previous FIFO sizes
        #ifdef BIDIRECTIONAL_ENDPOINTS
        //TODO
        #else
        uint8_t fifo_size_temp[CONFIG_USBDEV_EP_NUM - 1];

        for (uint8_t i = 0; i < CONFIG_USBDEV_EP_NUM - 1; i++)
        {
          fifo_size_temp[i] = 0;
        }

        for (usbd_endpoint& ep : endpoints)
        {
          uint8_t fifo_ptr = (ep.ep_addr % 0x80) - 1;
          if (fifo_ptr < CONFIG_USBDEV_EP_NUM - 1)
          {
            fifo_size_temp[fifo_ptr] = fifo_size[fifo_ptr];
          }
        }

        for (uint8_t i = 0; i < CONFIG_USBDEV_EP_NUM - 1; i++)
        {
          fifo_size[i] = fifo_size_temp[i];
        }
        #endif

        // MLOGV("USB", "Post cleanup FIFO sizes");
        // for (uint8_t i = 0; i < CONFIG_USBDEV_EP_NUM - 1; i++)
        // {
        //   MLOGV("USB", "FIFO-0%d size  %d", i + 1, fifo_size[i]);
        // }

        usbd_desc_register(USB_BUS_ID, desc);

        MLOGV("USB", "Registering USB descriptor (REG BASE: %p)", USBD_BASE);

        usbd_initialize(USB_BUS_ID, USBD_BASE, DeviceEventHandler);

        // clean up temp vectors
        usb_endpoint_descs.clear();
    }

    void End(void) {
        usbd_deinitialize(USB_BUS_ID);
    }

    void Connect(void) {
    }

    void Disconnect(void) {
    }
    
    usbd_interface* AddInterface() {
        usbd_interface intf;
        interfaces.push_back(intf);
        usbd_interface* intf_ptr = &interfaces.back();
        usbd_add_interface(USB_BUS_ID, intf_ptr);
        return intf_ptr;
    }

    usbd_endpoint* AddEndpoint(EndpointType type, usbd_endpoint_callback cb, uint16_t ep_size)
    {
        usbd_endpoint ep;
        ep.ep_cb = cb;
        
        usbd_endpoint* ep_ptr;

        #ifdef BIDIRECTIONAL_ENDPOINTS
        if (type == EndpointType::In)
        {
            ep.ep_addr = 0x80 + in_endpoints.size() + 1;  // Starts from 0x81
            if (in_endpoints.size() >= USB_MAX_IN_EP)
            {
                MatrixOS::SYS::ErrorHandler("Maximum number of USB IN endpoints reached");
                return nullptr;
            }
            in_endpoints.push_back(ep);
            ep_ptr = &in_endpoints.back();
            
            // TODO fix this for bidirectional endpoints
            fifo_size[(ep_ptr->ep_addr % 0x80) - 1] = 0; // No FIFO for IN endpoints
        }
        else if (type == EndpointType::Out)
        {
            ep.ep_addr = out_endpoints.size() + 1;  // Starts from 0x01
            if (in_endpoints.size() >= USB_MAX_OUT_EP)
            {
                MatrixOS::SYS::ErrorHandler("Maximum number of USB OUT endpoints reached");
                return nullptr;
            }
            out_endpoints.push_back(ep);
            ep_ptr = &out_endpoints.back();

            // TODO fix this for bidirectional endpoints
            fifo_size[(ep_ptr->ep_addr % 0x80) - 1] = ep_size;
        }
        #else
        if (type == EndpointType::In)
        {   
            ep.ep_addr = 0x80 + endpoints.size() + 1;  // Starts from 0x81
            if (endpoints.size() >= USB_MAX_IN_EP)
            {
                MatrixOS::SYS::ErrorHandler("Maximum number of USB IN endpoints reached");
                return nullptr;
            }
            endpoints.push_back(ep);
            ep_ptr = &endpoints.back();

            fifo_size[(ep_ptr->ep_addr % 0x80) - 1] = ep_size;
        }
        else if (type == EndpointType::Out)
        {
            ep.ep_addr = endpoints.size() + 1;  // Starts from 0x01
            if (endpoints.size() >= USB_MAX_OUT_EP)
            {
                MatrixOS::SYS::ErrorHandler("Maximum number of USB OUT endpoints reached");
                return nullptr;
            }
            endpoints.push_back(ep);
            ep_ptr = &endpoints.back();

            fifo_size[(ep_ptr->ep_addr % 0x80) - 1] = ep_size;
        }
        #endif

        usbd_add_endpoint(USB_BUS_ID, ep_ptr);
        // printf("Added endpoint 0x%02X\n", ep_ptr->ep_addr);
        return ep_ptr;
    }

    void AddInterfaceDescriptor(const vector<uint8_t>& desc)
    {
        // ESP_LOGI("USB", "Adding interface descriptor: ");
        // for (uint8_t i = 0; i < desc.size(); i++)
        // {
        //   printf("%02X ", desc[i]);
        // }
        // printf("\n");
        usb_interface_descs.push_back(desc);
    }

    uint8_t AddString(const string& str) {
        ESP_LOGI("USB", "Adding string: %s", str.c_str());

        if (str.empty())
        return 0;  // Uses default product name string

        // Check if exists
        int index = 0;
        for (auto it = string_desc_arr.begin(); it != string_desc_arr.end(); ++it, ++index) {
            if (*it == str) {
                return index + 3; // Found the string, add offset for the first 3 strings
            }
        }

        string_desc_arr.push_back(str);
        return static_cast<uint8_t>(string_desc_arr.size() - 1 + 3);  // Add offset for the first 3 strings
    }

    uint8_t* CompileDescriptor() {
        // Calculate the size of the USB configuration descriptor
        uint16_t usb_desc_size = 9 /*USB_CONFIG_DESCRIPTOR_INIT*/;

        for (vector<uint8_t> desc : usb_interface_descs)
        {
            usb_desc_size += desc.size();
        }

        // Calculate the size of the USB string descriptor
        uint16_t usb_string_size = 4; // USB_LANGID_INIT
        for (string str : string_desc_arr)
        {
            usb_string_size += str.length() * 2 + 2; // 2 bytes of header + 2 bytes per character
        }

        usb_descriptor.clear();
        usb_descriptor.reserve(18 /*USB_DEVICE_DESCRIPTOR_INIT*/ + usb_desc_size + usb_string_size);
        usb_descriptor.insert(usb_descriptor.end(), {USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, Device::usb_vid, Device::usb_pid, 0x0100, 0x01)}); // size 18
        MLOGV("USB", "Inserted USB descriptor (USB_DEVICE_DESCRIPTOR_INIT) size: %d", usb_descriptor.size());
        usb_descriptor.insert(usb_descriptor.end(), {USB_CONFIG_DESCRIPTOR_INIT(usb_desc_size, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER)}); // size 9
        MLOGV("USB", "Inserted USB descriptor (USB_CONFIG_DESCRIPTOR_INIT) size: %d", usb_descriptor.size());

        // Insert interface descriptor
        for (vector<uint8_t> desc : usb_interface_descs)
        {
            usb_descriptor.insert(usb_descriptor.end(), desc.begin(), desc.end());
            MLOGV("USB", "Inserted USB descriptor (interface) size: %d", usb_descriptor.size());
        }        

        // Insert String descriptors

        // Language ID
        usb_descriptor.insert(usb_descriptor.end(), {USB_LANGID_INIT(USBD_LANGID_STRING)});
        MLOGV("USB", "Inserted USB descriptor (USB_LANGID_INIT) size: %d", usb_descriptor.size());

        auto insertStrToDesc = [&](const string& str) {
            usb_descriptor.insert(usb_descriptor.end(), str.length() * 2 + 2);
            usb_descriptor.insert(usb_descriptor.end(), USB_DESCRIPTOR_TYPE_STRING);
            for (char c : str)
            {
                usb_descriptor.push_back(c);
                usb_descriptor.push_back(0);
            }
            MLOGV("USB", "Inserted USB descriptor (string %s) size: %d", str.c_str(), usb_descriptor.size());
        };

        string product_name = Device::product_name;
        // TODO not working pls fix
        // if (MatrixOS::UserVar::device_id.Get())
        // {
        //     product_name += " ";
        //     product_name += std::to_string(MatrixOS::UserVar::device_id.Get());
        // }


        insertStrToDesc(Device::manufacturer_name);
        insertStrToDesc(product_name);
        insertStrToDesc(Device::GetSerial());

        for (string str : string_desc_arr)
        {
            insertStrToDesc(str);
        }

        // print out the descriptor
        MLOGV("USB", "Descriptor length: %d", usb_descriptor.size());
        // for (uint16_t i = 0; i < usb_descriptor.size(); i++)
        // {
        //     MLOGV("USB", "%02X ", usb_descriptor[i]);
        // }

        return usb_descriptor.data();
    }

    // Status
    bool Mounted(void)
    {
        return mounted;
    }

    bool Suspended(void)
    {
        return suspended;
    }

    bool Ready(void)
    {
        return inited && connected && !suspended;
        // return inited && mounted && !suspended; // TODO should be mounted but idk how to get that in CherryUSB
    }

    bool RemoteWakeup(void)
    {
        return remoteWakeup;
    }
 
    void DeviceEventHandler(uint8_t busid, uint8_t event)
    {
        switch (event) {
            /* USB DCD IRQ */
            case USBD_EVENT_ERROR:
                break;
            case USBD_EVENT_RESET:
                break;
            case USBD_EVENT_SOF:
                break;
            case USBD_EVENT_CONNECTED:
                MLOGV("USB", "USB connected");
                connected = true;
                break;
            case USBD_EVENT_DISCONNECTED:
                MLOGV("USB", "USB disconnected");
                connected = false;
                break;
            case USBD_EVENT_RESUME:
                MLOGV("USB", "USB resumed");
                suspended = false;
                break;
            case USBD_EVENT_SUSPEND:
                MLOGV("USB", "USB suspended");
                suspended = true;
                break;
            /* USB DEVICE STATUS */
            case USBD_EVENT_CONFIGURED:
                break;
            case USBD_EVENT_SET_INTERFACE:
                break;
            case USBD_EVENT_SET_REMOTE_WAKEUP:
                MLOGV("USB", "Remote wakeup set");
                remoteWakeup = true;
                break;
            case USBD_EVENT_CLR_REMOTE_WAKEUP:
                MLOGV("USB", "Remote wakeup cleared");
                remoteWakeup = false;
                break;
            case USBD_EVENT_INIT:
                MLOGV("USB", "USB inited");
                inited = true;
                break;
            case USBD_EVENT_DEINIT:
                MLOGV("USB", "USB deinited");
                inited = false;
                break;
            default:
                break;
        }
    }
}