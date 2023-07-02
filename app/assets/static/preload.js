const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
    listen: (event, handler) => ipcRenderer.on(event, (event, data) => {
        if (typeof handler === 'function') {
            handler(data, event);
        }
    }),
    send: (message, data) => {
        ipcRenderer.send(message, data);
    },
    invoke: (message, data = {}, callback = null) => {
        if (typeof data === 'function') {
            callback = data;
            data = {};
        }
        ipcRenderer.invoke(message, data)
            .then(res => {
                if (typeof callback === 'function') {
                    callback(res);
                }
            })
            .catch(err => console.log(err));
    }
});