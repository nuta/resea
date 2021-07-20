use crate::{
    capi::{ipc_lookup, task_t},
    cstring::CString,
    result::Result,
    stub_helpers::ClientBase,
    task::Task,
    try_capi,
};

pub struct Client {
    server_task: Task,
}

impl Client {
    pub fn lookup(service_name: &str) -> Result<Client> {
        let cstr = CString::from_str(service_name);
        let server_task = unsafe { Task::from_id(try_capi!(ipc_lookup(cstr.as_cstr()))) };
        Ok(Client { server_task })
    }
}

impl ClientBase for Client {
    fn server_task(&self) -> task_t {
        self.server_task.id()
    }
}
