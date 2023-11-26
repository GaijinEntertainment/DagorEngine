import { CoreService } from "./core.service.js";
import { StreamService } from "./stream.service.js";
import { SettingsService } from "./settings.service.js";
import { ECSService } from "./ecs.service.js";
import { DMService } from "./dm.service.js";
import { useCookies } from "../vendor/vue-cookies.js";

const { cookies } = useCookies();

cookies.put = cookies.set;

export const cookieService = cookies;
export const settingsService = new SettingsService();
export const streamService = new StreamService(settingsService);
export const coreService = new CoreService(streamService, cookies);
export const ecsService = new ECSService(coreService);
export const dmService = new DMService(coreService);

export default {
  settingsService,
  streamService,
  coreService,
  ecsService,
  dmService,
  cookieService,
}