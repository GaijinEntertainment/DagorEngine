Removes the debug agent with the given category name.  Notifies all other agents via `onUninstall`, then destroys the agent and its context.  Safe no-op if the agent does not exist.
