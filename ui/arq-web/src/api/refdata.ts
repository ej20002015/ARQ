import { httpClient } from "./core/httpClient";
import { type RefDataMetadataResponse } from "@/features/refdata/types/RefDataFieldMetadata";

export const RefDataAPI = {
    getEntities: () =>
        httpClient.get<string[]>("refdata/entities")
            .then(res => res.data),

    getRecords: (entityName: string) =>
        httpClient.get<any[]>(`refdata/records/${entityName}`)
            .then(res => res.data),

    getSchema: (entityName: string) =>
        httpClient.get<RefDataMetadataResponse>(`refdata/metadata/${entityName}`)
            .then(res => res.data),
}